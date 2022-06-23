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

struct flock lock;

extern int errno;

sem_t* w_sem_open(char* sem_name, int initial_val){
    sem_t * sem_ptr = sem_open(sem_name, O_RDWR | O_CREAT, 0666, initial_val);
    if(sem_ptr == SEM_FAILED){
        perror("sem_open 0");
        exit(1);
    }
    return sem_ptr;
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

int main(int argc, char *argv[]){

    char* filename = argv[1];
    char* shm_name = argv[2];
    // char* shared_memory_addr = argv[3];
    char* wholesaler_s = argv[4];
    char* chef_0_s = argv[5];
    char* chef_1_s = argv[6];
    char* chef_2_s = argv[7];
    char* chef_3_s = argv[8];
    char* chef_4_s = argv[9];
    char* chef_5_s = argv[10];
    char* flour_pusher_s = argv[11];
    char* walnut_pusher_s = argv[12];
    char* sugar_pusher_s = argv[13];
    char* milk_pusher_s = argv[14];
    char* shm_bool_name = argv[15];
    char* shm_bool_addr = argv[16];
    char* mutex_name = "mutex";

    sem_unlink(wholesaler_s);
    sem_unlink(chef_0_s);
    sem_unlink(chef_1_s);
    sem_unlink(chef_2_s);
    sem_unlink(chef_3_s);
    sem_unlink(chef_4_s);
    sem_unlink(chef_5_s);
    sem_unlink(flour_pusher_s);
    sem_unlink(walnut_pusher_s);
    sem_unlink(sugar_pusher_s);
    sem_unlink(milk_pusher_s);

    sem_t* wholesaler_sem = sem_open(wholesaler_s, O_RDWR | O_CREAT, 0666, 1);

    w_sem_open(chef_0_s, 0);
    w_sem_open(chef_1_s, 0);
    w_sem_open(chef_2_s, 0);
    w_sem_open(chef_3_s, 0);
    w_sem_open(chef_4_s, 0);
    w_sem_open(chef_5_s, 0);

    sem_t* flour_sem = w_sem_open(flour_pusher_s, 0);
    sem_t* walnut_sem = w_sem_open(walnut_pusher_s, 0);
    sem_t* sugar_sem = w_sem_open(sugar_pusher_s, 0);
    sem_t* milk_sem = w_sem_open(milk_pusher_s, 0);

    w_sem_open(mutex_name, 1);


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

    char* argv_chef0[] = {
        "./chef", shm_name, shared_memory, wholesaler_s, chef_0_s, 
        "Walnuts", "Sugar", "", "", "0", NULL
    };

    char* argv_chef1[] = {
        "./chef", shm_name, shared_memory, wholesaler_s, chef_1_s,
        "Walnuts", "Flour", "", "", "1", NULL
    };

    char* argv_chef2[] = {
        "./chef", shm_name, shared_memory, wholesaler_s, chef_2_s, 
        "Sugar", "Flour", "", "", "2", NULL
    };

    char* argv_chef3[] = {
        "./chef", shm_name, shared_memory, wholesaler_s, chef_3_s, 
        "Milk", "Flour", "", "", "3", NULL
    };

    char* argv_chef4[] = {
        "./chef", shm_name, shared_memory, wholesaler_s, chef_4_s, 
        "Milk", "Walnuts", "", "", "4", NULL
    };

    char* argv_chef5[] = {
        "./chef", shm_name, shared_memory, wholesaler_s, chef_5_s, 
        "Sugar", "Milk", "", "", "5", NULL
    };

    char** argv_chef[] = { argv_chef0, argv_chef1, argv_chef2, argv_chef3, argv_chef4, argv_chef5 };

    // flour - walnut - sugar - milk
    char* argv_flour_pusher[] = {
        "./pusher", flour_pusher_s, chef_1_s, chef_2_s, chef_3_s, "1", "2", "3", "0", 
        shm_bool_name, shm_bool_addr, mutex_name, NULL
    };

    char* argv_walnut_pusher[] = {
        "./pusher", walnut_pusher_s, chef_1_s, chef_0_s, chef_4_s, "0", "2", "3", "1", 
        shm_bool_name, shm_bool_addr, mutex_name, NULL
    };

    char* argv_sugar_pusher[] = {
        "./pusher", sugar_pusher_s, chef_2_s, chef_0_s, chef_5_s, "0", "1", "3", "2", 
        shm_bool_name, shm_bool_addr, mutex_name, NULL
    };

    char* argv_milk_pusher[] = {
        "./pusher", milk_pusher_s, chef_3_s, chef_4_s, chef_5_s, "0", "1", "2", "3", 
        shm_bool_name, shm_bool_addr, mutex_name, NULL
    };

    char** argv_pusher[] = { argv_flour_pusher, argv_walnut_pusher, argv_sugar_pusher, argv_milk_pusher };

    pid_t pid_chef[CHEF_SIZE];
    for(int i = 0; i<CHEF_SIZE; i++){
        pid_chef[i] = fork();
        if(pid_chef[i] == 0){
            execv("./chef", argv_chef[i]);
            perror("execv chef");
            exit(1);
        }
    }

    pid_t pid_pusher[PUSHER_SIZE];
    for(int i=0; i<PUSHER_SIZE; ++i){
        pid_pusher[i] = fork();
        if(pid_pusher[i] == 0){
            execv("./pusher", argv_pusher[i]);
            perror("execv pisher");
            exit(1);
        }
    }

    char* tmp_addr = shared_memory;
    int ingredient_fd = open(filename, O_RDONLY, 0666);
    if (ingredient_fd == -1) {
        perror("data file");
        exit(1);
    }

    int read_byte = 0;
    int b_print = 0;
    memset(&lock, 0, sizeof(lock));

    while (1){
        
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
        while((read_byte = read(ingredient_fd, tmp_addr, sizeof(char) * 3)) && (errno == EINTR)){
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
            char first_ing = *(tmp_addr);
            switch (first_ing)
            {
                case 'F':   sem_post(flour_sem);
                    break;
                case 'S':   sem_post(sugar_sem);
                    break;
                case 'M':   sem_post(milk_sem);
                    break;
                case 'W':   sem_post(walnut_sem);
                    break;
                default:
                    perror("Unknown ingredient");
                    exit(1);
            }
            char second_ing = *(tmp_addr+1);
            switch (second_ing)
            {
                case 'F':   sem_post(flour_sem);
                    break;
                case 'S':   sem_post(sugar_sem);
                    break;
                case 'M':   sem_post(milk_sem);
                    break;
                case 'W':   sem_post(walnut_sem);
                    break;
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
        kill(pid_pusher[i], SIGKILL);
    
    for(int i=0; i<CHEF_SIZE; ++i)
        kill(pid_chef[i], SIGINT);

    int sum = 0;
    int ret_val;
    for(int i=0; i<CHEF_SIZE; ++i){
        waitpid(pid_chef[i], &ret_val, 0);
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
        kill(pid_chef[i], SIGKILL);
    
    munmap(shared_memory, sizeof(char) * 2048);
    close(ingredient_fd);
    sem_close(wholesaler_sem);
    sem_close(flour_sem);
    sem_close(sugar_sem);
    sem_close(milk_sem);
    sem_close(walnut_sem);

    sem_unlink(wholesaler_s);
    sem_unlink(chef_0_s);
    sem_unlink(chef_1_s);
    sem_unlink(chef_2_s);
    sem_unlink(chef_3_s);
    sem_unlink(chef_4_s);
    sem_unlink(chef_5_s);
    sem_unlink(flour_pusher_s);
    sem_unlink(walnut_pusher_s);
    sem_unlink(sugar_pusher_s);
    sem_unlink(milk_pusher_s);
    
    return 0;
}   