#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>


#include "structs.h"

#define LOG_FILE "client_logs"

// ./client -s pathToServerFifo -o data.csv

int main(int argc, char*argv[]){

    
    if(argc < 5){
        perror("Not same number of arguments");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout,"Client Id %d\n", getpid());

    char res_name_fifo[20] = {0};
    sprintf(res_name_fifo, "%d",getpid());

    if(mkfifo(res_name_fifo, 0666) == -1){
        perror("Child fifo creation failed ");
        exit(EXIT_FAILURE);
    }

    time_t t = time(NULL);
    struct tm tm;

    char* server_fifo_path = argv[2];
    char* data_file_path = argv[4];

    int buffer_i[1024] = {0};
    int line_num=0;

    int data_fd;
    if((data_fd = open(data_file_path, O_RDONLY)) == -1){
        perror("Data file opening failed");
        exit(EXIT_FAILURE);
    }

    int read_byte_d, 
        total_byte;
    
    int num = 0;
    int index=0;
    while(1){
        char ch;
        while ((read_byte_d = read(data_fd, &ch, 1)) == -1){
            if(errno == EINTR){
                continue;
            }
            perror("Data file reading failed");
            exit(EXIT_FAILURE);
        }
        if(read_byte_d == 0){
            buffer_i[index] = num;
            line_num++;
            break;
        }
        total_byte += read_byte_d;
        if(ch == ',' || ch == '\n'){
            buffer_i[index++] = num;
            num = 0;
            if(ch == '\n')
                line_num++;
            continue;
        }
        num = num * 10 + (ch - '0');
    }
    if(close(data_fd) == -1){
        perror("Data file closing failed");
        exit(EXIT_FAILURE);
    }

    request request;
    request.client_id = getpid();
    request.matrix.size = line_num;
    for(int i = 0; i<line_num; ++i){
        for(int j=0; j<line_num; ++j){
            request.matrix.array[i*line_num+j] = buffer_i[i*line_num+j];
        }
    }


    int server_fd = open(server_fifo_path, O_WRONLY);
    if(server_fd  == -1){
        perror("Server fifo opening failed client");
        exit(EXIT_FAILURE);
    }

    if(write(server_fd, &request, sizeof(request)) == -1){
        perror("Server fifo writing failed");
        exit(EXIT_FAILURE);
    }

    if(close(server_fd) == -1){
        perror("Server fifo closing failed");
        exit(EXIT_FAILURE);
    }

    int result_fd = open(res_name_fifo, O_RDONLY);
    if(result_fd == -1){
        perror("Result fifo opening failed");
        exit(EXIT_FAILURE);
    }

    int read_byte;
    response res;
    while((read_byte = read(result_fd, &res, sizeof(res))) == -1){
        if(errno == EINTR){
            continue;
        }
        perror("Result fifo reading failed");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    memset(&lock, 0, sizeof(lock));

    int log_fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_APPEND, 0666);
    if(log_fd == -1){
        perror("Child log file opened error");
        exit(EXIT_FAILURE);
    }

    char* result = (res.result < 0) ? ("not invertible") : ("invertible");
    char write_text[512] = {0};

    tm = *localtime(&t);
    sprintf(write_text, "%d-%02d-%02d %02d:%02d:%02d: Client PID#%d (%s)  is submitting a %dx%d  matrix\nClient PID#%d: the matrix is %s, total time %ld.%ld seconds, goodbye\n"
        ,tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
        ,getpid(), data_file_path, line_num, line_num, getpid(), result, res.sec, res.msec);

    lock.l_type = F_WRLCK;
    fcntl(log_fd, F_SETLKW, &lock);
    
    if(write(log_fd, write_text, strlen(write_text)) == -1){
        perror("Writing log file failed");
        exit(EXIT_FAILURE);
    }
    
    lock.l_type = F_UNLCK;        
    fcntl(log_fd, F_SETLKW, &lock);

    fprintf(stdout,"Result : %d\n", res.result);
    fprintf(stdout,"Result recorded client_logs file\n");

    if(close(result_fd) == -1){
        perror("Result fifo closing failed");
        exit(EXIT_FAILURE);
    }

    if(close(log_fd) == -1){
        perror("Log file closing failed");
        exit(EXIT_FAILURE);
    }

    if(unlink(res_name_fifo) == -1){
        perror("Result fifo unlinking failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}