#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>


int main(int argc, char*argv[]){

    char* core_ing = argv[1];
    char* first_chef = argv[2];
    char* second_chef = argv[3];
    char* third_chef = argv[4];

    int first_offset = (int) argv[5][0] - '0';
    int second_offset = (int) argv[6][0] - '0';
    int third_offset = (int) argv[7][0] - '0';
    int fourth_offset = (int) argv[8][0] - '0';

    char* shm_bool_name = argv[9];
    // char* shm_bool_addr = argv[10];

    char* mutex_name = argv[11];

    char* first_offset_ptr;
    char* second_offset_ptr;
    char* third_offset_ptr;
    char* fourth_offset_ptr;

    char T = 'T';
    char F = 'F';


    sem_t* ingredient = sem_open(core_ing, O_RDWR);
    if(ingredient == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    sem_t* chef_1 = sem_open(first_chef, O_RDWR);
    if(chef_1 == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    sem_t* chef_2 = sem_open(second_chef, O_RDWR);
    if(chef_2 == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    sem_t* chef_3 = sem_open(third_chef, O_RDWR);
    if(chef_3 == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    sem_t* mutex = sem_open(mutex_name, O_RDWR);
    if(mutex == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
    
    int fd = shm_open(shm_bool_name,  O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    char* shared_memory = (char*) mmap(NULL, sizeof(char) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    first_offset_ptr = (shared_memory + (first_offset * sizeof(char)));
    second_offset_ptr = (shared_memory + (second_offset * sizeof(char)));
    third_offset_ptr = (shared_memory + (third_offset * sizeof(char)));
    fourth_offset_ptr = (shared_memory + (fourth_offset * sizeof(char)));

    for(;;){

        sem_wait(ingredient);
        sem_wait(mutex);

        if(*first_offset_ptr == T){
            *first_offset_ptr = F;
            sem_post(chef_1);
        }else if(*second_offset_ptr == T){
            *second_offset_ptr = F;
            sem_post(chef_2);
        }else if(*third_offset_ptr == T){
            *third_offset_ptr = F;
            sem_post(chef_3);
        }else{
            *fourth_offset_ptr = T;
        }
        sem_post(mutex);
    }

    munmap(shared_memory, sizeof(char) * 4);
    shm_unlink(shm_bool_name);
    sem_close(ingredient);
    sem_close(chef_1);
    sem_close(chef_2);
    sem_close(chef_3);
    sem_close(mutex);

    sem_unlink(core_ing);
    sem_unlink(first_chef);
    sem_unlink(second_chef);
    sem_unlink(third_chef);
    sem_unlink(mutex_name);

    return 0;
}