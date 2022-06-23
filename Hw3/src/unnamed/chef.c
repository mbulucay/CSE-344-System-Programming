#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>

struct flock lock;


sig_atomic_t chef_sigint = 0;

void handle_sig(int sig){
    chef_sigint++;
}


char* getValue(char key){

    char* ret;
    switch (key){
        case 'W':   ret = "Walnuts";    break;
        case 'M':   ret = "Milk";       break;
        case 'F':   ret = "Flour";      break;
        case 'S':   ret = "Sugar";      break;
        default:
            printf("UNKNOWN:%c:\n", key);
            exit(1);
    }

    return ret;
}

void pop_shared_memory(char* shared_memory_addr, int nth){

    lock.l_type = F_WRLCK;
    fcntl(0, F_SETLKW, &lock);

    while(*(shared_memory_addr+1) != '\0'){
        
        char* iter = shared_memory_addr;
        fprintf(stdout, "Content of array: ");
        while(*iter!= '\0'){
            fprintf(stdout, "%c", *iter);
            iter++;
        }

        fprintf(stdout, "Chef%d (pid %d) has taken the %s\n", nth, getpid(), getValue(*shared_memory_addr));
        *shared_memory_addr = '\0';
        shared_memory_addr++;
    }
    fprintf(stdout,"Chef%d (pid %d) preparing the dessert\n", nth, getpid());
    fflush(stdout);

    lock.l_type = F_UNLCK;
    fcntl(0, F_SETLKW, &lock);
}


int main(int argc, char* argv[]){


    int nth = argv[1][0] - '0';
    char* lack_of_ing_1 = argv[2];
    char* lack_of_ing_2 = argv[3];
    char* shm_name = argv[4];
    char* shm_sem_name = argv[6];

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &handle_sig; 

    if((sigaction(SIGINT, &sa, NULL) == -1)){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    int fd = shm_open(shm_name, O_RDWR, 0666);
    if(fd == -1){
        perror("shm_open");
        exit(1);
    }

    char* shared_memory = (char*) mmap(NULL, sizeof(char) * 2048, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shared_memory == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    fd = shm_open(shm_sem_name, O_RDWR, 0666);
    if(fd == -1){
        perror("shm_open");
        exit(1);
    }

    sem_t* shm_sem_addr = (sem_t*) mmap(NULL, sizeof(sem_t) * 12, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shm_sem_addr == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    sem_t* chef_sem = &shm_sem_addr[nth];
    sem_t* whole_sem = &shm_sem_addr[6];

    int counter = 0;
    for(;;){

        lock.l_type = F_WRLCK;
        fcntl(0, F_SETLKW, &lock);
        fprintf(stdout,"Chef%d (pid %d) is waiting for %s and %s\n", nth, getpid(), lack_of_ing_1, lack_of_ing_2);
        fflush(stdout);
        lock.l_type = F_UNLCK;
        fcntl(0, F_SETLKW, &lock);
        
        sem_wait(chef_sem);
        
        if(chef_sigint)
            break;
    
        pop_shared_memory(shared_memory, nth);
        counter++;

        lock.l_type = F_WRLCK;
        fcntl(0, F_SETLKW, &lock);
        fprintf(stdout,"Chef%d (pid %d) delivered the dessert\n", nth, getpid());
        fflush(stdout);
        lock.l_type = F_UNLCK;
        fcntl(0, F_SETLKW, &lock);

        sem_post(whole_sem);
    }

    munmap(shared_memory, sizeof(char) * 2048);
    munmap(shm_sem_addr, sizeof(sem_t) * 12);
    shm_unlink(shm_name);
    shm_unlink(shm_sem_name);

    return counter;
}
