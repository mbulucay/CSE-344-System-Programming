#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>


#define SERVER_LOG "server_logs"


#include "structs.h"
#include "matrix_math.h"

int enter_read = 1;
sem_t* sem_read;
sem_t* sem_cont;

sem_t* working_child;

sem_t* z_inverted;
sem_t* z_not_inverted;

sem_t* number_of_inverted;
sem_t* number_of_not_inverted;

pid_t pid_arr[100];
int num_of_ids = 0;

int child_fifo_fd;
int child_log_fd;

int server_pipe_in;

void handle_sig_child(int sig){

    // fprintf(stdout,"Entered back_up_server child %d\n", getpid());

    if(close(child_fifo_fd) == -1)
        write(child_log_fd,"Child fifo close failed in child It could be already closed\n", strlen("Child fifo close failed in child It could be already closed\n"));
    if(close(child_log_fd) == -1)
        write(child_log_fd,"Child log close failed in child It could be already closed\n", strlen("Child log close failed in child It could be already closed\n"));

    exit(EXIT_SUCCESS);
}


void in_child(request* req, int id, int pool_size, int duration){

    response res;
    request req_local;
    struct timeval start, end;

    time_t t = time(NULL);
    struct tm tm;

    char fifo_name[50] = {0};
    char write_text[512] = {0};
    int worker_num;
    
    sigset_t set_SIGINT, prev;
    sigemptyset(&set_SIGINT);
    sigaddset(&set_SIGINT, SIGINT);
    
    struct flock lock;
    memset(&lock, 0, sizeof(lock));

    for(;;){
        
        sem_wait(sem_read);     // buradan sonra memory degerleri local req variable icine kopyalaniyor
        
        gettimeofday(&start, NULL);
        
        req_local.client_id = req->client_id;
        req_local.matrix.size = req->matrix.size;
        for(int i=0; i<req->matrix.size; i++){
            for(int j=0; j<req->matrix.size; j++){
                req_local.matrix.array[i*req_local.matrix.size + j] = req->matrix.array[i*req->matrix.size + j];
            }
        }
        
        sem_post(sem_cont);     // serverZ shared memory icine yeni deger koyabilir
        
        sem_post(working_child);
        if(sem_getvalue(working_child, &worker_num) == -1){
            write(child_log_fd,"Sem get value failed\n", strlen("Sem get value failed\n"));
            exit(EXIT_FAILURE);
        }

        tm = *localtime(&t);
        sprintf(write_text, "%d-%02d-%02d %02d:%02d:%02d: Z:Worker PID#%d is handling client PID#%d, matrix size %dx%d, pool busy %d/%d\n"
            ,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
            , getpid(), req->client_id, req->matrix.size, req->matrix.size, worker_num, pool_size);

        lock.l_type = F_WRLCK;
        fcntl(child_log_fd, F_SETLKW, &lock);
        while((write(child_log_fd, write_text, strlen(write_text)) == -1) && errno == EINTR){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Writing log file failed\n", strlen("Writing log file failed\n"));
            exit(EXIT_FAILURE);
        }
        
        lock.l_type = F_UNLCK;        
        fcntl(child_log_fd, F_SETLKW, &lock);


        if(sigprocmask(SIG_BLOCK, &set_SIGINT, &prev))
            write(child_log_fd,"Signal masking failed\n", strlen("Signal masking failed\n"));
        
        res.result = is_invertable(&req_local.matrix);
        if(res.result == 1){
            sem_post(number_of_inverted);
            sem_post(z_inverted);
        }else{
            sem_post(number_of_not_inverted);
            sem_post(z_not_inverted);
        }
        res.worker_number = id;

        if(sigprocmask(SIG_SETMASK, &prev, NULL))
            write(child_log_fd,"Signal masking failed\n", strlen("Signal masking failed\n"));

        sprintf(fifo_name, "%d", req_local.client_id);
        while(((child_fifo_fd = open(fifo_name, O_WRONLY)) == -1) && (errno == EINTR)){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Child fifo opening failed\n", strlen("Child fifo opening failed\n"));
            exit(EXIT_FAILURE);
        }
        sleep(duration);
        gettimeofday(&end, NULL);
        // res.time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
        res.sec = end.tv_sec - start.tv_sec;
        res.msec = end.tv_usec - start.tv_usec;

        while((write(child_fifo_fd, &res, sizeof(response)) == -1) && errno == EINTR){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Child fifo write failed\n", strlen("Child fifo write failed\n"));
            exit(EXIT_FAILURE);
        }

        tm = *localtime(&t);
        sprintf(write_text, "%d-%02d-%02d %02d:%02d:%02d: Z:Worker PID#%d responding to client PID#%d: the matrix %s.\n"
            ,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
            , getpid(), req->client_id, res.result == 1 ? "is invertable" : "is not invertable");

        lock.l_type = F_WRLCK;
        fcntl(child_log_fd, F_SETLKW, &lock);
        
        while((write(child_log_fd, write_text, strlen(write_text)) == -1) && errno == EINTR){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Writing log file failed\n", strlen("Writing log file failed\n"));
            exit(EXIT_FAILURE);
        }
        
        lock.l_type = F_UNLCK;        
        fcntl(child_log_fd, F_SETLKW, &lock);

        sem_wait(working_child);

    }
    exit(EXIT_SUCCESS);
}


void handle_sig(int sig){

    char write_text[512];
    int inverted, not_inverted;
    if(sem_getvalue(z_inverted, &inverted)==-1)
        write(child_log_fd,"Sem get value failed\n", strlen("Sem get value failed\n"));

    if(sem_getvalue(z_not_inverted, &not_inverted)==-1)
        write(child_log_fd,"Sem get value failed\n", strlen("Sem get value failed\n"));

    time_t t = time(NULL);
    struct tm tm;
    tm = *localtime(&t);
    sprintf(write_text, "%d-%02d-%02d %02d:%02d:%02d: Z:SIGINT received, exiting server Z. Total requests handled %d, %d invertible, %d not.\n"
        ,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
        ,inverted + not_inverted, inverted, not_inverted);

    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    fcntl(child_log_fd, F_SETLKW, &lock);
    
    while((write(child_log_fd, write_text, strlen(write_text)) == -1) && errno == EINTR){
        if(errno == EINTR)
            continue;
        write(child_log_fd,"Writing log file failed\n", strlen("Writing log file failed\n"));
        exit(EXIT_FAILURE);
    }
    
    lock.l_type = F_UNLCK;        
    fcntl(child_log_fd, F_SETLKW, &lock);


    if(sem_close(sem_read) == -1)
        write(child_log_fd,"Semaphore close failed sem_read\n", strlen("Semaphore close failed sem_read\n"));

    if(sem_unlink("read_sema") == -1)
        write(child_log_fd,"Semaphore unlink failed read_sema\n", strlen("Semaphore unlink failed read_sema\n"));

    if(sem_close(sem_cont) == -1)
        write(child_log_fd,"Semaphore close failed sem_cont\n", strlen("Semaphore close failed sem_cont\n"));

    if(sem_unlink("cont_sema") == -1)
        write(child_log_fd,"Semaphore unlink failed cont_sema\n", strlen("Semaphore unlink failed cont_sema\n"));

    if(sem_close(working_child) == -1)
        write(child_log_fd,"Semaphore close failed working_child\n", strlen("Semaphore close failed working_child\n"));
    
    if(sem_unlink("working_child_z") == -1)
        write(child_log_fd,"Semaphore unlink failed working_sema\n", strlen("Semaphore unlink failed working_sema\n"));

    if(sem_close(z_inverted) == -1)
        write(child_log_fd,"Semaphore close failed z_inverted\n", strlen("Semaphore close failed z_inverted\n"));
    
    if(sem_unlink("z_inverted_sema") == -1)
        write(child_log_fd,"Semaphore unlink failed z_inverted_sema\n", strlen("Semaphore unlink failed z_inverted_sema\n"));
    
    if(sem_close(z_not_inverted) == -1)
        write(child_log_fd,"Semaphore close failed z_not_inverted\n", strlen("Semaphore close failed z_not_inverted\n"));
    
    if(sem_unlink("z_not_inverted_sema") == -1)
        write(child_log_fd,"Semaphore unlink failed z_not_inverted_sema\n", strlen("Semaphore unlink failed z_not_inverted_sema\n"));

    if(shm_unlink("/servers_mbu") == -1)
        write(child_log_fd,"Shared memory unlink failed /servers_mbu\n", strlen("Shared memory unlink failed /servers_mbu\n"));

    if(close(server_pipe_in) == -1)
        write(child_log_fd,"Server pipe close failed in serverZ\n", strlen("Server pipe close failed in serverZ\n"));

    for(int i=0; i<num_of_ids; ++i){
        kill(pid_arr[i], SIGINT);
        wait(NULL);
    }

    exit(EXIT_SUCCESS);
}


void serverZ(int pool_size, int duration){

    int size = sizeof(request);

    int fd = shm_open("/servers_mbu", O_RDWR | O_CREAT, 0777);
    if(fd == -1){
        write(child_log_fd,"shm_open failed\n", strlen("shm_open failed\n"));
        exit(EXIT_FAILURE);
    }

    if(ftruncate(fd, 1024 * size) == -1){
        write(child_log_fd,"ftruncate failed\n", strlen("ftruncate failed\n"));
        exit(EXIT_FAILURE);
    }

    request* addr = (request*) mmap(NULL, 1024 * size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED){
        write(child_log_fd,"mmap failed\n", strlen("mmap failed\n"));
        exit(EXIT_FAILURE);
    }

    int id = 0;
    pid_t child_pid = 1;
    for(int i=0; i<pool_size; ++i){
        child_pid = fork();
        pid_arr[num_of_ids++] = child_pid;
        if(child_pid == 0){
            id = i+1;
            break;
        }
        if(child_pid == -1){
            write(child_log_fd,"Forking failed\n", strlen("Forking failed\n"));
            exit(EXIT_FAILURE);
        }
    }

    if(child_pid == 0){
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = &handle_sig_child; 
        if((sigaction(SIGINT, &sa, NULL) == -1)){
            write(child_log_fd,"Sigaction SIGINT\n", strlen("Sigaction SIGINT\n"));
            exit(EXIT_FAILURE);
        }
        in_child(addr, id, pool_size, duration);

    }else{
        
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = &handle_sig; 
        if((sigaction(SIGINT, &sa, NULL) == -1)){
            write(child_log_fd,"Sigaction SIGINT\n", strlen("Sigaction SIGINT\n"));
            exit(EXIT_FAILURE);
        }
        
        request req;
        int read_byte = 0, total_byte = 0;

        for(;;){

            read_byte = total_byte = 0;
            while(1){
                if((read_byte = read(server_pipe_in, &req, sizeof(request))) == -1){
                    if(errno == EINTR){
                        continue;
                    }
                    write(child_log_fd,"Server fifo reading failed\n", strlen("Server fifo reading failed\n"));
                    exit(EXIT_FAILURE);
                }
                if(read_byte == 0){
                    break;
                }
                total_byte += read_byte;
                if(total_byte == sizeof(request))
                    break;                
            }
       
            memcpy(addr, &req, sizeof(request));
            sem_post(sem_read);
            sem_wait(sem_cont);
        }
    }

}

int main(int argc, char*argv[]){

    int pool_size = atoi(argv[1]);
    server_pipe_in = atoi(argv[2]); 
    int duration = atoi(argv[3]);
    child_log_fd = atoi(argv[4]);

    sem_read = sem_open("read_sema", O_CREAT, 0666, 0);
    if(sem_read == SEM_FAILED){
        write(child_log_fd,"Semaphore open failed read_sema\n", strlen("Semaphore open failed read_sema\n"));
        exit(EXIT_FAILURE);
    }
    sem_cont = sem_open("cont_sema", O_CREAT, 0666, 0);
    if(sem_cont == SEM_FAILED){
        write(child_log_fd,"Semaphore open failed cont_sema\n", strlen("Semaphore open failed cont_sema\n"));
        exit(EXIT_FAILURE);
    }
    working_child = sem_open("working_child_z", O_CREAT, 0666, 0);
    if(working_child == SEM_FAILED){
        write(child_log_fd,"Semaphore open failed working_sema\n", strlen("Semaphore open failed working_sema\n"));
        exit(EXIT_FAILURE);
    }

    z_inverted = sem_open("z_inverted_sema", O_CREAT, 0666, 0);
    if(z_inverted == SEM_FAILED){
        write(child_log_fd,"Semaphore open failed z_inverted\n", strlen("Semaphore open failed z_inverted\n"));
        exit(EXIT_FAILURE);
    }

    z_not_inverted = sem_open("z_not_inverted_sema", O_CREAT, 0666, 0);
    if(z_not_inverted == SEM_FAILED){
        write(child_log_fd,"Semaphore open failed z_not_inverted_sema\n", strlen("Semaphore open failed z_not_inverted_sema\n"));
        exit(EXIT_FAILURE);
    }

    number_of_inverted = sem_open("inverted_sem", O_CREAT, 0777, 0);
    if(number_of_inverted == SEM_FAILED){
        write(child_log_fd,"Semaphore creation failed inverted_sem", strlen("Semaphore creation failed inverted_sem"));
        exit(EXIT_FAILURE);
    }
    number_of_not_inverted = sem_open("not_inverted_sem", O_CREAT, 0777, 0);
    if(number_of_not_inverted == SEM_FAILED){
        write(child_log_fd,"Semaphore creation failed not_inverted_sem", strlen("Semaphore creation failed not_inverted_sem"));
        exit(EXIT_FAILURE);
    }

    serverZ(pool_size, duration);

    return 0;
}