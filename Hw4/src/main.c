#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>

#include "semun.h"

#define SEM_NUM 2

// 1212121212121122112121121122121221122112222112121121222212112211212121211122121212121121122122112122
// 2222111122221111221122111122221122221111212122111212122222111122221111221122111122221122221111212211

typedef struct _supplier_args
{
    char* filename;
    int sem_id;
}_supplier_args;


typedef struct _consumer_arg
{
    int nth;
    int sem_id;
}_consumer_arg;


int N, C, sem_id = -1;
union semun smn_set;
struct sembuf sembuf[SEM_NUM];
struct flock lock;
char current_time_str[150] = { 0 };

int fd = -1;
int* nth_consumer = NULL;
pthread_t *consumer_thread = NULL;
_supplier_args* sup_args = NULL; 
sig_atomic_t is_exit = 0;

void SIGINT_handler(int signo){
    
    is_exit = 1;

    if(consumer_thread != NULL)
        free(consumer_thread);

    if(sup_args != NULL)
        free(sup_args);

    if(nth_consumer != NULL)
        free(nth_consumer);

    if(fd != -1)
        close(fd);
    if(sem_id != -1)
        semctl(sem_id, 0, IPC_RMID);

    for(int i=0; i<C; ++i)
        pthread_join(consumer_thread[i], NULL);

    exit(1);
}


void set_current_time(){

    int milli_sec;
    char str_time[100];
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    milli_sec = ( current_time.tv_usec / 1000 );

    strftime(str_time, 100, "%H:%M:%S", localtime(&current_time.tv_sec));
    sprintf(current_time_str, "%s:%03d", str_time, milli_sec);
}


void* supplier(void* arg){
    
    _supplier_args* params = (_supplier_args*)arg;

    if((fd = open(params->filename, O_RDWR, 0777)) == -1){
        perror("open file");
        exit(1);
    }

    for(;;){
        int read_byte;
        char read_char;
        while((read_byte = read(fd, &read_char, sizeof(char))) && errno == EINTR){
            if(errno == EINTR)
                continue;
            perror("read");
            exit(1);
        }

        if(read_byte == 0)
            break;
        if(read_char == '\n')
            break;
        /////////////////
        // sleep(2);
        
        if(read_byte == -1){
            perror("read");
            exit(1);
        }

        if(is_exit)
            break;

        lock.l_type = F_WRLCK;
        fcntl(0, F_SETLKW, &lock);
        
        set_current_time();
        fprintf(stdout, "%s ", current_time_str);
        fprintf(stdout, "Supplier: read input a '%c'. Current Amounts %d x '1'  %d x '2'.\n", read_char, (smn_set.array[0]) / 2, (smn_set.array[1]) / 2);
        
        lock.l_type = F_UNLCK;
        fcntl(0, F_SETLKW, &lock);

        switch (read_char)
        {
            case '1':
                sembuf[0].sem_op = 1;
                if(semop(sem_id, &sembuf[0], 1) == -1){
                    perror("semop");
                    exit(1);
                }   
                break;
            case '2':
                sembuf[1].sem_op = 1;
                if(semop(sem_id, &sembuf[1], 1) == -1){
                    perror("semop");
                    exit(1);
                }   


                break;
            default:
                fprintf(stdout, "Unknown character type");
                exit(1);
        }

        if(is_exit)
            break;

        if(semctl(sem_id, 0, GETALL, smn_set) == -1){
            perror("semctl");
            exit(1);
        }

        lock.l_type = F_WRLCK;
        fcntl(0, F_SETLKW, &lock);

        set_current_time();
        fprintf(stdout, "%s ", current_time_str);
        fprintf(stdout, "Supplier: delivered a '%c'. Post-delivery amounts: %d x '1'  %d x '2'.\n", read_char, (smn_set.array[0] + 1) / 2, (smn_set.array[1] + 1) /2);

        lock.l_type = F_UNLCK;
        fcntl(0, F_SETLKW, &lock);


        if(is_exit)
            break;

        switch (read_char)
        {
            case '1':
                sembuf[0].sem_op = 1;
                if(semop(sem_id, &sembuf[0], 1) == -1){
                    perror("semop");
                    exit(1);
                }   
                break;
            case '2':
                sembuf[1].sem_op = 1;
                if(semop(sem_id, &sembuf[1], 1) == -1){
                    perror("semop");
                    exit(1);
                }   
                break;
            default:
                fprintf(stdout, "Unknown character type");
                exit(1);
        }

        if(semctl(sem_id, 0, GETALL, smn_set) == -1){
            perror("semctl");
            exit(1);
        }        
        
        if(is_exit)
            break;

    } 



    if(close(fd) == -1){
        perror("close");
        exit(1);
    }

    fprintf(stdout, "The Supplier has left.\n");

    return NULL;
}


void* consumer(void* arg){
    

    int id = *(int*)arg;
    free(arg);

    for(int i=0; i<N; ++i){

        lock.l_type = F_WRLCK;
        fcntl(0, F_SETLKW, &lock);

        set_current_time();
        fprintf(stdout, "%s ", current_time_str);
        fprintf(stdout, "Consumer-%d at iteration %d (waiting) Current amounts: %d x '1'  %d x '2'.\n", id, i, (smn_set.array[0] + 1) / 2, (smn_set.array[1] + 1) / 2);
        
        lock.l_type = F_UNLCK;
        fcntl(0, F_SETLKW, &lock);

        if(is_exit)
            break;
        
        /////////////////
        // sleep(2);

        sembuf[0].sem_op = -2;
        sembuf[1].sem_op = -2;

        if(semop(sem_id, sembuf, SEM_NUM) == -1){
            perror("semop");
            exit(1);
        }
        if(semctl(sem_id, 0, GETALL, smn_set) == -1){
            perror("semctl");
            exit(1);
        }
        if(is_exit)
            break;

        lock.l_type = F_WRLCK;
        fcntl(0, F_SETLKW, &lock);

        set_current_time();
        fprintf(stdout, "%s ", current_time_str);
        fprintf(stdout, "Consumer-%d at iteration %d (consumed) Post-consumption amounts: %d x '1' %d x '2'.\n", id, i, (smn_set.array[0] + 1) / 2, (smn_set.array[1] + 1) / 2);

        lock.l_type = F_UNLCK;
        fcntl(0, F_SETLKW, &lock);

        if(is_exit)
            break;

    }

    fprintf(stdout, "Consumer-%d has left\n", id);

    return NULL;
}


int main(int argc, char* argv[]){


    if(argc != 7){
        fprintf(stderr, "Usage: ./hw4 -C <num_of_consumers> -N <num_of_iterations> -F <file_path>\n");
        exit(1);
    }

    setbuf(stdout, NULL);
    memset(&lock, 0, sizeof(lock));
    
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &SIGINT_handler;
    if((sigaction(SIGINT, &sa, NULL) == -1))
        perror("Sigaction");
    

    sem_id = semget(IPC_PRIVATE, SEM_NUM,  S_IRUSR | S_IWUSR);
    if(sem_id == -1){
        perror("semget");
        exit(1); 
    }

    short unsigned int arr[] = {0, 0};
    smn_set.array = arr;

    if(semctl(sem_id, 0, SETALL, smn_set) == -1){
        perror("semctl");
        exit(1);
    }

    sembuf[0].sem_num = 0;
    sembuf[0].sem_flg = 0;
    
    sembuf[1].sem_num = 1;
    sembuf[1].sem_flg = 0;

    C = atoi(argv[2]);
    N = atoi(argv[4]);

    pthread_t supplier_thread;
    consumer_thread = (pthread_t*) calloc(sizeof(pthread_t), C);

    sup_args = (_supplier_args*) calloc(sizeof(_supplier_args), 1);
    sup_args->filename = argv[6];
    // sup_args->sem_id = sem_id;

    for(int i=0; i<C; ++i){
        nth_consumer = (int*) calloc(sizeof(int), 1);
        *nth_consumer = i;
        if(pthread_create(&consumer_thread[i], NULL, consumer, (void*) nth_consumer) != 0){
            perror("pthread_create");
            return -1;
        }

    }

    if(pthread_create(&supplier_thread, NULL, supplier, (void*) sup_args) != 0){
        perror("pthread_create");
        return -1;
    }

    if(pthread_detach(supplier_thread) != 0){
        perror("pthread_detach");
        return -1;
    }

    for(int i=0; i<C; ++i){
        if(pthread_join(consumer_thread[i], NULL) != 0){
            perror("pthread_join");
            return -1;
        }
    }

    free(consumer_thread);
    free(sup_args);

    if (semctl(sem_id, 0, IPC_RMID) == -1){    
        perror("semctl-GETALL");
        return -1;
    }

    pthread_exit(0);

    return 0;
}
