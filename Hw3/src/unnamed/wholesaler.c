#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>

#define CHEF_SIZE 6
#define PUSHER_SIZE 4

struct flock lock;

char* SHARED_MEM = "/named_shm_mbu";
char* SHARED_BOOL = "/bool_values";

char* SEMAPHORE_SHARED_MEM = "/semaphores_shm_mbu";

pid_t chef_pids[CHEF_SIZE];
pid_t pusher_pids[PUSHER_SIZE];


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

int main(int argc, char* argv[]){

    char* filename = argv[1];
    char* shared_memory = argv[2];
    char* shared_bool = argv[3];
    char* semaphore_shared_memory = argv[4];


    char* argv_chef0[] = {
        "./chef", "0", "Walnuts", "Sugar",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_chef1[] = {
        "./chef", "1", "Walnuts", "Flour",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_chef2[] = {
        "./chef", "2", "Sugar", "Flour",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_chef3[] = {
        "./chef", "3", "Flour", "Milk",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_chef4[] = {
        "./chef", "4", "Walnuts", "Milk",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_chef5[] = {
        "./chef", "5", "Sugar", "Milk",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char** argv_chefs[] = { argv_chef0, argv_chef1, argv_chef2, argv_chef3, argv_chef4, argv_chef5 };

    pid_t pid;
    for(int i = 0; i < 6; i++) {
        pid = fork();
        chef_pids[i] = pid;
        if(pid == 0) {
            execv("./chef", argv_chefs[i]);
            perror("execv");
            exit(1);
        }
    }

    // flour - walnut - sugar - milk
    char* argv_pusher0[] = {
        "./pusher", "Flour",
        "8",                    // produtc semaphore index
        "1", "2", "3",          // chefs semaphore indexes
        "1","2","3","0",        // boolean queue indexes
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_pusher1[] = {
        "./pusher", "Walnuts",
        "9",    
        "1", "0", "4", 
        "0","2","3","1",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_pusher2[] = {
        "./pusher", "Sugar",
        "10",
        "2", "0", "5",
        "0","1","3","2",
        SHARED_MEM,SHARED_BOOL,SEMAPHORE_SHARED_MEM,
        NULL
    };

    char* argv_pusher3[] = {
        "./pusher", "Milk",
        "11",   
        "3", "4", "5",
        "0","1","2","3",
        SHARED_MEM, SHARED_BOOL, SEMAPHORE_SHARED_MEM,
        NULL
    };

    char** argv_pushers[] = { argv_pusher0, argv_pusher1, argv_pusher2, argv_pusher3 };
    
    for(int i = 0; i < 4; i++) {
        pid = fork();
        pusher_pids[i] = pid;
        if(pid == 0) {
            execv("./pusher", argv_pushers[i]);
            perror("execv");
            exit(1);
        }
    }

    int fd = shm_open(SHARED_MEM,  O_RDWR | O_CREAT , 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }
    char* shared_memory_ptr = (char*) mmap(NULL, sizeof(char) * 2048, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    fd = shm_open(SHARED_BOOL,  O_RDWR | O_CREAT , 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }
    char* shared_bool_ptr = (char*) mmap(NULL, sizeof(char) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_bool_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    fd = shm_open(SEMAPHORE_SHARED_MEM,  O_RDWR | O_CREAT , 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }
    sem_t* semaphore_shared_memory_ptr = (sem_t*) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (semaphore_shared_memory_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    sem_t* wholesaler_sem = &semaphore_shared_memory_ptr[6];    
    sem_t* flour_sem = &semaphore_shared_memory_ptr[8];         
    sem_t* walnut_sem = &semaphore_shared_memory_ptr[9];         
    sem_t* sugar_sem = &semaphore_shared_memory_ptr[10];         
    sem_t* milk_sem = &semaphore_shared_memory_ptr[11];          

    char* tmp_addr = shared_memory_ptr;
    int ingredient_fd = open(filename, O_RDONLY, 0666);
    if (ingredient_fd == -1) {
        perror("data file");
        exit(1);
    }

    int read_byte = 0;
    int b_print = 0;

    while(1){

        if(b_print != 0){
            lock.l_type = F_WRLCK;
            fcntl(0, F_SETLKW, &lock);
            fprintf(stdout, "The wholesaler (pid %d) waiting for the dessert\n", getpid());
            fflush(stdout);
            lock.l_type = F_UNLCK;
            fcntl(0, F_SETLKW, &lock);
        }

        sem_wait(wholesaler_sem);

        if(b_print != 0){
            lock.l_type = F_WRLCK;
            fcntl(0, F_SETLKW, &lock);
            fprintf(stdout, "The wholesaler (pid %d) has obtained the dessert and left\n",getpid());
            fflush(stdout);
            lock.l_type = F_UNLCK;
            fcntl(0, F_SETLKW, &lock);
        }

        while((read_byte = read(ingredient_fd, tmp_addr, sizeof(char) * 3)) && (errno == EINTR) ){
            if(errno == EINTR)
                continue;
            perror("read");
            exit(1);
        }
        if(read_byte == 0)
            break;

        if(read_byte == -1){
            perror("read");
            exit(1);
        }

        if(read_byte >= 2){

            b_print = 1;
            char first_ing = tmp_addr[0];
            switch (first_ing)
            {
                case 'F':  sem_post(flour_sem);      break;
                case 'S':  sem_post(sugar_sem);      break;
                case 'M':  sem_post(milk_sem);       break;
                case 'W':  sem_post(walnut_sem);     break;
                default:
                    perror("Unknown ingredient");
                    exit(1);
            }

            char second_ing = tmp_addr[1];
            switch (second_ing)
            {
                case 'F':   sem_post(flour_sem);    break;
                case 'S':   sem_post(sugar_sem);    break;
                case 'M':   sem_post(milk_sem);     break;
                case 'W':   sem_post(walnut_sem);   break;
                default:
                    perror("Unknown ingredient");
                    exit(1);
            }
            lock.l_type = F_WRLCK;
            fcntl(0, F_SETLKW, &lock);
            fprintf(stdout, "The wholesaler (pid %d) has delivered %s %s\n", getpid(), getValue(first_ing), getValue(second_ing));
            fflush(stdout);
            lock.l_type = F_UNLCK;
            fcntl(0, F_SETLKW, &lock);
        }
    }

    for(int i=0; i<PUSHER_SIZE; ++i)
        kill(pusher_pids[i], SIGKILL);
    
    for(int i=0; i<CHEF_SIZE; ++i)
        kill(chef_pids[i], SIGINT);

    int sum = 0;
    int ret_val;
    for(int i=0; i<CHEF_SIZE; ++i){
        waitpid(chef_pids[i], &ret_val, 0);
        if(ret_val == -1){
            perror("waitpid");
            exit(1);
        }
        sum += WEXITSTATUS(ret_val);
    }

    lock.l_type = F_WRLCK;
    fcntl(0, F_SETLKW, &lock);
    fprintf(stdout, "The wholesaler (pid %d) is done (total dessert : %d)\n", getpid(), sum);
    fflush(stdout);
    lock.l_type = F_UNLCK;
    fcntl(0, F_SETLKW, &lock);

    for(int i=0; i<CHEF_SIZE; ++i)
        kill(chef_pids[i], SIGKILL);

    munmap(shared_memory_ptr, sizeof(char) * 2048);
    munmap(shared_bool_ptr, sizeof(char) * 4);
    munmap(semaphore_shared_memory_ptr, sizeof(sem_t));
    close(ingredient_fd);
    shm_unlink(shared_memory);
    shm_unlink(shared_bool);
    shm_unlink(semaphore_shared_memory);

    sem_close(wholesaler_sem);
    sem_close(flour_sem);
    sem_close(walnut_sem);
    sem_close(sugar_sem);
    sem_close(milk_sem);
    

    return 0;
}
