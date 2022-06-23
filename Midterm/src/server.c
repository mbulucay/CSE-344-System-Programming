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
#include <semaphore.h>
#include <time.h>

#include "structs.h"
#include "matrix_math.h"
#include "b_deamon.h"

#define SERVERS_PATH "servers_path"
#define SERVER_LOG "server_logs"

sig_atomic_t sigint = 0;
sig_atomic_t sigint_child = 0;
sig_atomic_t working = 0;

sem_t* number_of_inverted;
sem_t* number_of_not_inverted;
sig_atomic_t forwarded_requests = 0;

sem_t* working_child;

pid_t pid_arr[100];
int num_of_ids = 0;

int log_file_fd;

int child_fifo_fd;
int child_log_fd;

int server_fifo_fd;

int servers_pipe[2];
int parent2child_p[2];

char* server_fifo_path;
char* log_fifo_path;

void handle_sig_child(int sig){

    // fprintf(stdout, "Child %d received signal %d\n", getpid(), sig);

    if(close(child_fifo_fd) == -1)
        write(child_log_fd,"Child fifo close failed in child It could be already closed\n", strlen("Child fifo close failed in child It could be already closed\n"));
    
    if(close(child_log_fd) == -1)
        write(child_log_fd,"Child log file close failed in child It could be already closed\n", strlen("Child log file close failed in child It could be already closed\n"));

    if(close(parent2child_p[0]) == -1)
        write(child_log_fd,"Parent2child pipe close failed in child It could be already closed\n", strlen("Parent2child pipe close failed in child It could be already closed\n"));

    if(close(parent2child_p[1]) == -1)
        write(child_log_fd,"Parent2child pipe close failed in child It could be already writen\n", strlen("Parent2child pipe close failed in child It could be already writen\n"));

    exit (EXIT_SUCCESS);
}


void in_child(int pipe[], int id, int pool_size, int duration){

    request req;
    response res;

    struct timeval start, end;
    time_t t = time(NULL);
    struct tm tm;

    char write_text[512] = {0};
    char fifo_name[50] = {0};
    int read_bytes ,worker_num;

    // struct timeval  tv;

    sigset_t set_SIGINT, prev;
    sigemptyset(&set_SIGINT);
    sigaddset(&set_SIGINT, SIGINT);
    
    struct flock lock;
    memset(&lock, 0, sizeof(lock));

    for(;;){
        
        // fprintf(stdout,"Z START %d\n", getpid());

        while((read_bytes = read(pipe[0], &req, sizeof(request))) == -1){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Reading from pipe failed\n", sizeof("Reading from pipe failed\n"));
            exit(EXIT_FAILURE);
        }

        kill(getppid(), SIGUSR1);
        gettimeofday(&start, NULL);


        sem_post(working_child);
        if(sem_getvalue(working_child, &worker_num) == -1){
            write(child_log_fd,"Sem get value failed\n", sizeof("Sem get value failed\n"));
            exit(EXIT_FAILURE);
        }

        tm = *localtime(&t);
        sprintf(write_text, "%d-%02d-%02d %02d:%02d:%02d: Y:Worker PID#%d is handling client PID#%d, matrix size %dx%d, pool busy %d/%d\n"
            ,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
            ,getpid(), req.client_id, req.matrix.size, req.matrix.size, worker_num, pool_size);

        lock.l_type = F_WRLCK;
        fcntl(child_log_fd, F_SETLKW, &lock);
        
        while((write(child_log_fd, write_text, strlen(write_text)) == -1) && (errno == EINTR)){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Writing log file failed\n", sizeof("Writing log file failed\n"));
            exit(EXIT_FAILURE);
        }
        
        lock.l_type = F_UNLCK;        
        fcntl(child_log_fd, F_SETLKW, &lock);

        if(sigprocmask(SIG_BLOCK, &set_SIGINT, &prev))
                write(child_log_fd,"Signal mask\n", sizeof("Signal mask\n"));

        res.result = is_invertable(&req.matrix);
        if(res.result == 1){
            sem_post(number_of_inverted);
        }else{
            sem_post(number_of_not_inverted);
        }
        res.worker_number = id;

        if(sigprocmask(SIG_SETMASK, &prev, NULL))
            write(child_log_fd,"Signal mask", sizeof("Signal mask"));

        sprintf(fifo_name, "%d", req.client_id);
        while(((child_fifo_fd = open(fifo_name, O_WRONLY)) == -1) && errno == EINTR){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Child fifo opening failed\n", sizeof("Child fifo opening failed\n"));
            exit(EXIT_FAILURE);
        }
        sleep(duration);
        
        gettimeofday(&end, NULL);
        res.sec = end.tv_sec - start.tv_sec;
        res.msec = end.tv_usec - start.tv_usec;

        while((write(child_fifo_fd, &res, sizeof(response)) == -1) && (errno == EINTR)){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Child fifo writing failed\n", sizeof("Child fifo writing failed\n"));
            exit(EXIT_FAILURE);
        }
        
        while((close(child_fifo_fd) == -1) && errno == EINTR){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Child fifo closing failed\n", sizeof("Child fifo closing failed\n"));
            exit(EXIT_FAILURE);
        }

        tm = *localtime(&t);
        sprintf(write_text, "%d-%02d-%02d %02d:%02d:%02d: Y:Worker PID#%d responding to client PID#%d: the matrix %s.\n"
            ,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
            , getpid(), req.client_id, res.result == 1 ? "is invertable" : "is not invertable");

        lock.l_type = F_WRLCK;
        fcntl(child_log_fd, F_SETLKW, &lock);
        
        while((write(child_log_fd, write_text, strlen(write_text)) == -1) && (errno == EINTR)){
            if(errno == EINTR)
                continue;
            write(child_log_fd,"Writing log file failed\n", sizeof("Writing log file failed"));
            exit(EXIT_FAILURE);
        }
        
        lock.l_type = F_UNLCK;        
        fcntl(child_log_fd, F_SETLKW, &lock);
        sem_wait(working_child);
        kill(getppid(), SIGUSR2);

    }

    exit(EXIT_SUCCESS);
}

char write_text_log[512];
int inverted, not_inverted;

void handle_sig(int signo){

    switch (signo)
    {   
        case SIGINT:
            
            child_log_fd = open(SERVER_LOG, O_CREAT | O_WRONLY | O_APPEND, 0666);
            if(child_log_fd == -1){
                exit(EXIT_FAILURE);
            }

            if(sem_getvalue(number_of_inverted, &inverted)==-1)
                write(child_log_fd,"Sem get value failed", sizeof("Sem get value failed"));

            if(sem_getvalue(number_of_not_inverted, &not_inverted)==-1)
                write(child_log_fd,"Sem get value failed", sizeof("Sem get value failed"));

            time_t t = time(NULL);
            struct tm tm;
            tm = *localtime(&t);
            sprintf(write_text_log, "%d-%02d-%02d %02d:%02d:%02d: SIGINT received, terminating Z and exiting server Y. Total requests handled: %d, %d invertible, %d not. %d requests were forwarded.\n"
                        ,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
                        ,inverted + not_inverted, inverted, not_inverted, forwarded_requests);
            
            
            struct flock lock;
            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            fcntl(child_log_fd, F_SETLKW, &lock);
            
            if(write(child_log_fd, write_text_log, strlen(write_text_log)) == -1){
                write(child_log_fd,"Writing log file failed", sizeof("Writing log file failed"));
                exit(EXIT_FAILURE);
            }
            
            lock.l_type = F_UNLCK;        
            fcntl(child_log_fd, F_SETLKW, &lock);

            if(sem_close(number_of_inverted) == -1)
                write(child_log_fd,"Semaphore close failed number_of_inverted\n", sizeof("Semaphore close failed number_of_inverted\n"));

            if(sem_unlink("inverted_sem") == -1)
                write(child_log_fd,"Semaphore unlink failed inverted_sem\n", sizeof("Semaphore unlink failed inverted_sem"));

            if(sem_close(number_of_not_inverted) == -1)
                write(child_log_fd,"Semaphore close failed number_of_not_inverted\n", sizeof("Semaphore close failed number_of_not_inverted\n"));
            
            if(sem_unlink("not_inverted_sem") == -1)
                write(child_log_fd,"Semaphore unlink failed not_inverted_sem\n", sizeof("Semaphore unlink failed not_inverted_sem\n"));
            
            if(sem_close(working_child) == -1)
                write(child_log_fd,"Semaphore close failed working_child\n", sizeof("Semaphore close failed working_child\n"));

            if(sem_unlink("working_child_y") == -1)
                write(child_log_fd,"Semaphore unlink failed working_child_y\n", sizeof("Semaphore unlink failed working_child_y\n"));

            if(unlink(server_fifo_path) == -1)
                write(child_log_fd,"Server fifo unlink failed It could be already unlinked\n", sizeof("Server fifo unlink failed It could be already unlinked\n"));

            if(close(servers_pipe[0]) == -1)
                write(child_log_fd,"Servers pipe closing failed It could be already closed\n", sizeof("Servers pipe closing failed It could be already closed\n"));

            if(close(parent2child_p[0]) == -1)
                write(child_log_fd,"Parent2child pipe closing failed It could be already closed\n", sizeof("Parent2child pipe closing failed It could be already closed\n"));

            if(close(parent2child_p[1]) == -1)
                write(child_log_fd,"Parent2child pipe closing failed It could be already closed\n", sizeof("Parent2child pipe closing failed It could be already closed\n"));

            for(int i; i<num_of_ids; i++){
                kill(pid_arr[i], SIGINT);
                wait(&pid_arr[i]);
            }
            exit(EXIT_SUCCESS);
            // break;
        case SIGUSR1:
            working++;
            break;   
        case SIGUSR2:
            working--;
            break;     
    }
}


void serverY(pid_t pid, char* server_fifo_path, char* log_fifo_path, int pool_size, int pool_size2, int duration){

    int backup_server_run = 0;
    int read_byte, 
        total_byte;
    request req;

    child_log_fd = open(SERVER_LOG, O_CREAT | O_WRONLY | O_APPEND, 0666);
    if(child_log_fd == -1){
        write(child_log_fd,"Server log file opened failed\n", sizeof("Server log file opened failed\n"));
        exit(EXIT_FAILURE);
    }

    if(mkfifo(server_fifo_path, 0777) == -1){
        write(child_log_fd,"Server fifo creation failed\n", sizeof("Server fifo creation failed\n"));
        exit(EXIT_FAILURE);
    }

    if(pipe(parent2child_p) == -1){
        write(child_log_fd,"Parent to child pipe creation failed\n", sizeof("Parent to child pipe creation failed\n"));
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
            write(child_log_fd,"Forking failed\n", sizeof("Forking failed\n"));
            exit(EXIT_FAILURE);
        }
    }

    if(child_pid == 0){

        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = &handle_sig_child; 
        if((sigaction(SIGINT, &sa, NULL) == -1)){
            write(child_log_fd,"Sigaction\n", sizeof("Sigaction\n"));
            exit(EXIT_FAILURE);
        }

        in_child(parent2child_p, id, pool_size, duration);

    }else{

        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = &handle_sig; 

        if((sigaction(SIGUSR1, &sa, NULL) == -1)){
            write(child_log_fd,"Sigaction SIGUSR1\n", sizeof("Sigaction SIGUSR1\n"));
            exit(EXIT_FAILURE);
        }
        if((sigaction(SIGUSR2, &sa, NULL) == -1)){
            write(child_log_fd,"Sigaction SIGUSR2\n", sizeof("Sigaction SIGUSR2\n"));
            exit(EXIT_FAILURE);
        }
        if((sigaction(SIGINT, &sa, NULL) == -1)){
            write(child_log_fd,"Sigaction SIGINT\n", sizeof("Sigaction SIGINT\n"));
            exit(EXIT_FAILURE);
        }
          

        for(;;){   

            int inverted_num = 0, not_inverted_num;
            if(sem_getvalue(number_of_inverted, &inverted_num)){
                write(child_log_fd,"Semaphore getvalue failed\n", sizeof("Semaphore getvalue failed\n"));
                exit(EXIT_FAILURE);
            }
            if(sem_getvalue(number_of_not_inverted, &not_inverted_num)){
                write(child_log_fd,"Semaphore getvalue failed\n", sizeof("Semaphore getvalue failed\n"));
                exit(EXIT_FAILURE);
            }

            while((server_fifo_fd = open(server_fifo_path, O_RDONLY)) == -1 && errno == EINTR){
                if(errno == EINTR)
                    continue;
                write(child_log_fd,"Server fifo opening failed\n", sizeof("Server fifo opening failed\n"));
                exit(EXIT_FAILURE);
            }

            while(1){
                if((read_byte = read(server_fifo_fd, &req, sizeof(request))) == -1){
                    if(errno == EINTR){
                        continue;
                    }
                    write(child_log_fd,"Server fifo reading failed\n", sizeof("Server fifo reading failed\n"));
                    exit(EXIT_FAILURE);
                }
                if(read_byte == 0){
                    break;
                }
                total_byte += read_byte;
                if(total_byte == sizeof(request))
                    break;                
            }

            if(working < pool_size){ 
                if(write(parent2child_p[1], &req, sizeof(request)) == -1){
                    write(child_log_fd,"Parent to child pipe writing failed\n", sizeof("Parent to child pipe writing failed\n"));
                    exit(EXIT_FAILURE);
                }
            }
            else{
                forwarded_requests++;
                if(backup_server_run == 0){
                    if(pipe(servers_pipe) == -1){
                        write(child_log_fd,"Server pipe creation failed\n", sizeof("Server pipe creation failed\n"));
                        exit(EXIT_FAILURE);
                    }

                    backup_server_run = 1;
                    pid_t backup_pid = fork();
                    pid_arr[num_of_ids++] = backup_pid;

                    if(backup_pid == -1){
                        write(child_log_fd,"Backup server creation failed\n", sizeof("Backup server creation failed\n"));
                        exit(EXIT_FAILURE);
                    }
                    if(backup_pid == 0){


                        if(close(servers_pipe[1])){
                            write(child_log_fd,"Server pipe closing failed\n", sizeof("Server pipe closing failed\n"));
                            exit(EXIT_FAILURE);
                        }

                        char poolsize_c[20] = {0};
                        sprintf(poolsize_c, "%d", pool_size2);
                        char file_descp[20] = {0};
                        sprintf(file_descp, "%d", servers_pipe[0]);
                        char duration_c[20] = {0};
                        sprintf(duration_c, "%d", duration);
                        char log_file[20] = {0};
                        sprintf(log_file, "%d", child_log_fd);

                        char* argv[] = {
                            "backup_server", 
                            poolsize_c, 
                            file_descp,
                            duration_c,
                            log_file,
                            NULL
                        };

                        execv("./backup_server", argv);
                        write(child_log_fd,"Backup server exec failed\n", sizeof("Backup server exec failed\n"));
                        exit(EXIT_FAILURE);
                    }else{
                        // if(close(servers_pipe[0])){
                        // write(child_log_fd,"Server pipe closing failed\n ", sizeof("Server pipe closing failed\n "));
                        //     exit(EXIT_FAILURE);
                        // }
                    }
                }
                if(write(servers_pipe[1], &req, sizeof(request)) == -1){
                    write(child_log_fd,"Backup server writing failed\n", sizeof("Backup server writing failed\n"));
                    exit(EXIT_FAILURE);
                }
            }

            if(close(server_fifo_fd) == -1){
                write(child_log_fd,"Server fifo closing failed\n", sizeof("Server fifo closing failed\n"));
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char*argv[]){

    if(argc != 11){
        exit(EXIT_FAILURE);
    } 

    int serverY_pool_size,
        serverZ_pool_size,
        duration;

    number_of_inverted = sem_open("inverted_sem", O_CREAT, 0777, 0);
    if(number_of_inverted == SEM_FAILED){
        exit(EXIT_FAILURE);
    }
    number_of_not_inverted = sem_open("not_inverted_sem", O_CREAT, 0777, 0);
    if(number_of_not_inverted == SEM_FAILED){
        exit(EXIT_FAILURE);
    }

    working_child = sem_open("working_child_y", O_CREAT, 0777, 0);
    if(working_child == SEM_FAILED){
        exit(EXIT_FAILURE);
    }

    becomeDaemon(BD_NO_CHDIR | BD_NO_UMASK0 | BD_NO_CLOSE_FILES);

    server_fifo_path = argv[2];
    log_fifo_path = argv[4];
    serverY_pool_size = atoi(argv[6]);
    serverZ_pool_size = atoi(argv[8]);
    duration = atoi(argv[10]);

    log_file_fd = open(SERVER_LOG, O_CREAT | O_WRONLY | O_APPEND, 0666);
    if(log_file_fd == -1){
        exit(EXIT_FAILURE);
    }

    if(serverY_pool_size < 1 || serverZ_pool_size < 1){
        write(child_log_fd,"Wrong arguments\n", sizeof("Wrong arguments\n"));
        exit(EXIT_FAILURE);
    }

    serverY(getpid(), server_fifo_path, log_fifo_path,serverY_pool_size, serverZ_pool_size, duration);

    return 0;
}