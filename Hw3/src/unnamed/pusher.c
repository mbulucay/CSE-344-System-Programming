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


int main(int argc, char* argv[]){

    int ingredient_offset = atoi(argv[2]);
    
    int chef1_offset = (int) argv[3][0] - '0';
    int chef2_offset = (int) argv[4][0] - '0';
    int chef3_offset = (int) argv[5][0] - '0';

    int bool1_offset = (int) argv[6][0] - '0';
    int bool2_offset = (int) argv[7][0] - '0';
    int bool3_offset = (int) argv[8][0] - '0';
    int bool4_offset = (int) argv[9][0] - '0';

    char* shm_bool_name = argv[11];
    char* shm_sem_name = argv[12];

    char True = 'T';
    char False = 'F';

    int fd = shm_open(shm_bool_name, O_RDWR, 0666);
    if(fd == -1) {
        perror("shm_open");
        exit(1);
    }
    char* shm_bool_addr = mmap(NULL, sizeof(char) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shm_bool_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    fd = shm_open(shm_sem_name, O_RDWR, 0666);
    if(fd == -1) {
        perror("shm_open");
        exit(1);
    }
    sem_t* shm_sem_addr =(sem_t*) mmap(NULL,  sizeof(sem_t) * 12, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shm_sem_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    sem_t* ingredient_sem = &shm_sem_addr[ingredient_offset];

    sem_t* mutex = &shm_sem_addr[7];

    sem_t* chef_1_sem = &shm_sem_addr[chef1_offset];
    sem_t* chef_2_sem = &shm_sem_addr[chef2_offset];
    sem_t* chef_3_sem = &shm_sem_addr[chef3_offset];

    char* bool_1_ing = &shm_bool_addr[bool1_offset];
    char* bool_2_ing = &shm_bool_addr[bool2_offset];
    char* bool_3_ing = &shm_bool_addr[bool3_offset];
    char* bool_4_ing = &shm_bool_addr[bool4_offset];


    for(;;){
        
        sem_wait(ingredient_sem);
        sem_wait(mutex);

        if(*bool_1_ing == True){
            *bool_1_ing = False;
            sem_post(chef_1_sem);
        }else if(*bool_2_ing == True){
            *bool_2_ing = False;
            sem_post(chef_2_sem);
        }else if(*bool_3_ing == True){
            *bool_3_ing = False;
            sem_post(chef_3_sem);
        }else{
            *bool_4_ing = True;
        }

        sem_post(mutex);
    }

    munmap(shm_bool_addr, sizeof(char) * 4);
    munmap(shm_sem_addr, sizeof(sem_t) * 12);
    shm_unlink(shm_bool_name);
    shm_unlink(shm_sem_name);

    return 0;
}