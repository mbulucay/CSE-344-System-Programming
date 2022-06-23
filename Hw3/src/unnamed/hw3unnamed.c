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

#define CHEF_SIZE 6
#define PUSHER_SIZE 4

char* SHARED_MEM = "/named_shm_mbu";
char* SHARED_BOOL = "/bool_values";

char* SEMAPHORE_SHRD_MEM = "/semaphores_shm_mbu";

pid_t wholesaler_pid;
pid_t pusher_pids[PUSHER_SIZE];


int main(int argc, char* argv[]){

    char* data =  argv[2];
    unlink(SHARED_MEM);
    unlink(SHARED_BOOL);
    unlink(SEMAPHORE_SHRD_MEM);

    int fd = shm_open(SHARED_MEM,  O_RDWR | O_CREAT , 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(fd, sizeof(char) * 2048)) {
        perror("ftruncate");
        exit(1);
    }
    char* shared_memory = (char*) mmap(NULL, sizeof(char) * 2048, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    for(int i = 0; i<2048; i++)
        shared_memory[i] = '\0';


    fd = shm_open(SHARED_BOOL,  O_RDWR | O_CREAT , 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }
    // flour - walnut - sugar - milk
    if (ftruncate(fd, sizeof(char) * 4)) {
        perror("ftruncate");
        exit(1);
    }
    char* shared_bool = (char*) mmap(NULL, sizeof(char) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_bool == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    *shared_bool = *(shared_bool + 1) = *(shared_bool + 2) = *(shared_bool + 3) = 'F';


    fd = shm_open(SEMAPHORE_SHRD_MEM, O_RDWR | O_CREAT, 0666);
    if(fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if(ftruncate(fd, sizeof(sem_t) * 12)) {
        perror("ftruncate");
        exit(1);
    }
    sem_t* semaphore_shm = (sem_t*) mmap(NULL, sizeof(sem_t) * 12, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (semaphore_shm == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    sem_init(&semaphore_shm[0], 1, 0);  // chef_0_s
    sem_init(&semaphore_shm[1], 1, 0);  // chef_1_s  
    sem_init(&semaphore_shm[2], 1, 0);  // chef_2_s
    sem_init(&semaphore_shm[3], 1, 0);  // chef_3_s
    sem_init(&semaphore_shm[4], 1, 0);  // chef_4_s
    sem_init(&semaphore_shm[5], 1, 0);  // chef_5_s

    sem_init(&semaphore_shm[6], 1, 1);  // wholesaler_s
    
    sem_init(&semaphore_shm[7], 1, 1);  // mutex
    sem_init(&semaphore_shm[8], 1, 0);  // flour_pusher_s
    sem_init(&semaphore_shm[9], 1, 0);  // walnut_pusher_s
    sem_init(&semaphore_shm[10], 1, 0); // sugar_pusher_s
    sem_init(&semaphore_shm[11], 1, 0); // milk_pusher_s


    char* argv_wholesaler[] = {
        "./wholesaler",
        data,
        SHARED_MEM,
        SHARED_BOOL,
        SEMAPHORE_SHRD_MEM,
        NULL
    };

    pid_t pid = fork();
    wholesaler_pid = pid;
    if(pid == 0){
        execv("./wholesaler", argv_wholesaler);
        perror("execv");
        exit(1);
    }

    wait(&wholesaler_pid);

    return 0;
}
