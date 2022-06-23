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
            ret = "Unknown key";
    }

    return ret;
}

// void printOnTerminal(char* sentence){

//     int bytes_written;
//     int size = sizeof(sentence);
    
//     lock.l_type = F_WRLCK;
//     fcntl(0, F_SETLKW, &lock);

//     while((bytes_written = write(0, sentence, strlen(sentence))) == -1 && errno == EINTR){

//     }

//     lock.l_type = F_UNLCK;
//     fcntl(0, F_SETLKW, &lock);

// }

void pop_shared_memory(char* shared_memory_addr, int nth){

    lock.l_type = F_WRLCK;
    fcntl(0, F_SETLKW, &lock);

    while(*(shared_memory_addr+1) != '\0'){
        
        char* iter = shared_memory_addr;
        fprintf(stdout, "Content of array: ");
        while(*iter != '\0'){
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

int main(int argc, char *argv[]){


    char* shm_name = argv[1];
    // char* shared_memory_addr = argv[2];
    char* wholesaler_s = argv[3];
    char* lack_ing_1 = argv[5];
    char* lack_ing_2 = argv[6];

    int nth = (int) argv[9][0] - '0';

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &handle_sig; 

    if((sigaction(SIGINT, &sa, NULL) == -1)){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }


    sem_t* chef_sem = sem_open(argv[4], O_RDWR);
    if(chef_sem == SEM_FAILED){
        perror("sem_open");
        exit(1);
    }

    sem_t* wholesaler_sem = sem_open(wholesaler_s, O_RDWR);
    if(wholesaler_sem == SEM_FAILED){
        perror("sem_open");
        exit(1);
    }

    int fd = shm_open(shm_name,  O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    char* shared_memory = (char*) mmap(NULL, sizeof(char) * 2048, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    int counter = 0;
    memset(&lock, 0, sizeof(lock));

    for(;;){

        lock.l_type = F_WRLCK;
        fcntl(0, F_SETLKW, &lock);
        fprintf(stdout,"Chef%d (pid %d) is waiting for %s and %s\n", nth, getpid(), lack_ing_1, lack_ing_2);
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

        sem_post(wholesaler_sem);
    }

    munmap(shared_memory, sizeof(char) * 2048);
    shm_unlink(shm_name);
    sem_close(chef_sem);
    sem_close(wholesaler_sem);

    return counter;
}   