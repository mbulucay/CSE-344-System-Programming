#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <math.h>
#include <signal.h>
#include <errno.h>

#include "syscall_wrappers.h"
#include "vector.h"

#define WRITE_FLAG (O_CREAT | O_WRONLY | O_APPEND)
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define VECTOR_SIZE 3
#define POINT_NUMBER 10
#define NUM_STR_LENGTH 20

int fd = -1;
char ** stringMatrix = NULL;

extern int errno;

char* num_to_string(double val);
char** calculate_covarience(char* numbers, Matrix* matrix);

sig_atomic_t is_get = 0;
void SIGINT_handler(int signo){
    is_get = 1;
}
void check_SIGINT();

void write_file(int fd, char** stringMatrix);



int main(int argc, char* argv[], char* env[]){

    sigset_t set_SIGINT, prev;
    sigemptyset(&set_SIGINT);
    sigaddset(&set_SIGINT, SIGINT);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &SIGINT_handler;

    if((sigaction(SIGINT, &sa, NULL) == -1))
        perror("Sigaction");
    
    //critic section
    if(sigprocmask(SIG_BLOCK, &set_SIGINT, &prev))
        perror("Signal mask");
    
    Matrix m;
    stringMatrix = calculate_covarience(env[0], &m);
    
    if(sigprocmask(SIG_SETMASK, &prev, NULL))
        perror("Signal mask");
    //critic section
    check_SIGINT();
    fd = w_open_p(argv[0], WRITE_FLAG, FILE_PERMISSIONS);

    write_file(fd, stringMatrix);

    check_SIGINT();
    w_close(fd);

    check_SIGINT();
    
    for(int i=0; i<VECTOR_SIZE * VECTOR_SIZE; ++i)
        free(stringMatrix[i]);
    free(stringMatrix);

    return 0;
}



void check_SIGINT(){

    if(is_get == 0) return;

    if(fd != -1)
        w_close(fd);
    
    if(stringMatrix != NULL){
        for(int i=0; i<VECTOR_SIZE * VECTOR_SIZE; ++i)
            free(stringMatrix[i]);
        free(stringMatrix);
    }

    exit(EXIT_SUCCESS);
}


/*
    Wrinting the calculated result to the result file
*/
void write_file(int fd, char** stringMatrix){

    struct flock lock;  

    memset(&lock , 0 ,sizeof(lock));
    lock.l_type = F_WRLCK;
    if(fcntl(fd,F_SETLKW,&lock) == -1)
        perror("fcntl");
    
    check_SIGINT();

    for(int i= 0; i<VECTOR_SIZE * VECTOR_SIZE; ++i){

        int bytes = strlen(stringMatrix[i]);
        int bytes_written = 0;

        while (bytes > 0){
            while((bytes_written = write(fd, stringMatrix[i], strlen(stringMatrix[i])))
                    && (errno == EINTR)){
                        continue;
            }
            if(bytes_written < 0){
                perror("write_file");
                exit(EXIT_FAILURE);
            }
            bytes -= bytes_written;
        }

        bytes = 1;
        bytes_written = 0;
        char* comma = ",";
        int comma_size = strlen(comma);    
        while((comma_size = write(fd, comma, comma_size) == -1) 
            && errno == EINTR){
               continue;
        }
        if(comma_size < 0){
            perror("write_file");
            exit(EXIT_FAILURE);
        }
    }
    
    char* new_line = "\n";
    int new_line_size = strlen(new_line);

    while((new_line_size = write(fd, new_line, new_line_size) == -1) 
            && errno == EINTR){
        continue;
    }
    if(new_line_size < 0){
        perror("write_file");
        exit(EXIT_FAILURE);
    }

    check_SIGINT();

    lock.l_type = F_UNLCK;
    if(fcntl(fd,F_SETLKW,&lock) == -1)
        perror("fcntl");

}


/*
    Calculating the covariance of the getting numbers
    assigning the matrix 
    and this area is critical reagion for the pointer
*/
char** calculate_covarience(char* numbers, Matrix* matrix){

    double x_sum = 0, y_sum = 0, z_sum = 0; 
    double x_mean = 0, y_mean = 0, z_mean = 0;
    double x_var = 0, y_var = 0, z_var = 0;
    double cov_xy = 0 , cov_xz = 0, cov_yz = 0;
    
    int i = 0;
    for(i = 0; i < POINT_NUMBER; ++i){
        x_sum += numbers[i * VECTOR_SIZE];
        y_sum += numbers[i * VECTOR_SIZE + 1];
        z_sum += numbers[i * VECTOR_SIZE + 2];
    }

    x_mean = x_sum / POINT_NUMBER;
    y_mean = y_sum / POINT_NUMBER;
    z_mean = z_sum / POINT_NUMBER;

    for(i = 0; i < POINT_NUMBER; ++i){
        x_var += pow(numbers[i * VECTOR_SIZE] - x_mean, 2);
        y_var += pow(numbers[i * VECTOR_SIZE + 1] - y_mean, 2);
        z_var += pow(numbers[i * VECTOR_SIZE + 2] - z_mean, 2);
    }

    x_var = x_var / POINT_NUMBER;
    y_var = y_var / POINT_NUMBER;
    z_var = z_var / POINT_NUMBER;

    for(i = 0; i < POINT_NUMBER; ++i){
        cov_xy += (numbers[i * VECTOR_SIZE] - x_mean) * (numbers[i * VECTOR_SIZE + 1] - y_mean);
        cov_xz += (numbers[i * VECTOR_SIZE] - x_mean) * (numbers[i * VECTOR_SIZE + 2] - z_mean);
        cov_yz += (numbers[i * VECTOR_SIZE + 1] - y_mean) * (numbers[i * VECTOR_SIZE + 2] - z_mean);
    }

    cov_xy = cov_xy / POINT_NUMBER;
    cov_xz = cov_xz / POINT_NUMBER;
    cov_yz = cov_yz / POINT_NUMBER;

    *matrix = createMatrix(x_var, y_var, z_var, cov_xy, cov_xz, cov_yz);

    

    char** cov = (char**) w_calloc(sizeof(char*), 9);

    cov[0] = num_to_string(x_var);
    cov[1] = num_to_string(cov_xy);
    cov[2] = num_to_string(cov_xz);
    cov[3] = num_to_string(cov_xy);
    cov[4] = num_to_string(y_var);
    cov[5] = num_to_string(cov_yz);
    cov[6] = num_to_string(cov_xz);
    cov[7] = num_to_string(cov_yz);
    cov[8] = num_to_string(z_var);

    return cov;
}


/*
    Converting the double to string
*/
char* num_to_string(double val){
    char* str = (char*) w_calloc(sizeof(char),NUM_STR_LENGTH);
    sprintf(str, "%lf", val);
    return str;
}
