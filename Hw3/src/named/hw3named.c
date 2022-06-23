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


char* SHARED_MEM = "/named_shm_mbu";
char* SHARED_BOOL = "/bool_values";

char* wholesaler_s; 

char* chef_0_s = "chef_0_s";
char* chef_1_s = "chef_1_s";
char* chef_2_s = "chef_2_s";
char* chef_3_s = "chef_3_s";
char* chef_4_s = "chef_4_s";
char* chef_5_s = "chef_5_s";

char* flour_pusher_s = "flour_push";
char* walnut_pusher_s = "walnut_push"; 
char* sugar_pusher_s = "sugar_push";
char* milk_pusher_s = "milk_push";


int main(int argc, char *argv[]){

    if(argc < 4){
        perror("Invalid arguments");
        exit(1);
    }

    char* data =  argv[2];
    wholesaler_s = argv[4];

    unlink(SHARED_MEM);
    unlink(SHARED_BOOL);

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


    fd = shm_open(SHARED_BOOL, O_RDWR | O_CREAT, 0666);
    if(fd == -1) {
        perror("shm_open");
        exit(1);
    }

    // flour - walnut - sugar - milk
    if(ftruncate(fd, sizeof(char) * 4)) {
        perror("ftruncate");
        exit(1);
    }
    char* shared_bool = (char*) mmap(NULL, sizeof(char) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shared_bool == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    *shared_bool = *(shared_bool + 1) = *(shared_bool + 2) = *(shared_bool + 3) = 'F';


    char* argv_wholesaler[] = {
        "./wholesaler",
        data,
        SHARED_MEM,
        shared_memory,
        wholesaler_s,
        chef_0_s,
        chef_1_s,
        chef_2_s,
        chef_3_s,
        chef_4_s,
        chef_5_s,
        flour_pusher_s,
        walnut_pusher_s,
        sugar_pusher_s,
        milk_pusher_s,
        SHARED_BOOL,
        shared_bool,
        NULL
    };

    pid_t pid_wholesaler = fork();
    if(pid_wholesaler == 0){
        execv("./wholesaler", argv_wholesaler);
        perror("execv");
        exit(1);
    }

    wait(NULL);

    return 0;
}   