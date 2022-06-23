#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <math.h>
// #include <sigaction.h>

#include "structs.h"

Matrix matrix_A, matrix_B, matrix_C;

pthread_mutex_t mutex_task_1;
pthread_cond_t cond_task_1;
sig_atomic_t task_1_done = 0;
sig_atomic_t sig_exit = 0;

int n, m;

void set_current_time(char arr[]){

    int milli_sec;
    char str_time[100];
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    milli_sec = ( current_time.tv_usec / 1000 );

    strftime(str_time, 100, "%H:%M:%S", localtime(&current_time.tv_sec));
    sprintf(arr, "%s:%03d", str_time, milli_sec);
}

void SIGINT_handler(int signo){
    sig_exit = 1;
    pthread_cond_broadcast(&cond_task_1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void read_file(char* filename, int length, char* content){

    int read_byte, 
        cpy_index = 0,
        total_byte = 0;
    char* buffer = (char*) calloc(sizeof(char), length);

    int fd;
    if((fd = open(filename, O_RDONLY)) == -1){
        perror("open");
        exit(1);
    }

    for(;;){
        
        if(sig_exit)
            break;

        for(int i=0; i<length; ++i)
            buffer[i] = '\0';

        while((read_byte = read(fd, buffer, sizeof(char) * length) > 0) && (errno == EINTR));
        if(read_byte == -1){
            perror("read");
            exit(1);
        }
        total_byte += strlen(buffer);

        if(read_byte == 0){
            if(total_byte >= length)    break;

            fprintf(stdout, "insufficient content %s ",filename);
            perror("fatal error");
            exit(1);
        }

        for(int i=0; i < length; ++i){
            content[cpy_index++] = buffer[i];
        }
    }

    free(buffer);

    if(close(fd) == -1){
        perror("close");
        exit(1);
    }

}

void write_file(char* filename, Matrix* real_matrix, Matrix* imag_matrix){
    
        if(real_matrix->matrix == NULL || imag_matrix->matrix == NULL || real_matrix == NULL || imag_matrix == NULL){
            fprintf(stdout, "real_matrix->n != imag_matrix->n\n");
            exit(1);
        }

        int fd;
        if((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
            perror("open");
            exit(1);
        }
    
        for(int i=0; i<real_matrix->n && !sig_exit; ++i){
            for(int j=0; j<real_matrix->n && !sig_exit; ++j){

                char real_str[100], imag_str[100];

                sprintf(real_str, "%lf", real_matrix->matrix[i][j]);
                sprintf(imag_str, "j(%lf)", imag_matrix->matrix[i][j]);

                while((write(fd, real_str, strlen(real_str)) > 0) && (errno == EINTR));
                while((write(fd, "+", 1) > 0) && (errno == EINTR));
                while((write(fd, imag_str, strlen(imag_str)) > 0) && (errno == EINTR));
                if(j != real_matrix->n - 1)
                    while((write(fd, ",", 1) > 0) && (errno == EINTR));
            }
            if(i != real_matrix->n - 1)
                while((write(fd, "\n", 1) > 0) && (errno == EINTR));
        }
    
        if(close(fd) == -1){
            perror("close");
            exit(1);
        }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

double* get_nth_row(Matrix* matrix,int nth){

    double* row = (double*) calloc(sizeof(double), matrix->n);

    for(int i=0; i<matrix->n; ++i)
        row[i] = matrix->matrix[nth][i];

    return row;
}

double* get_nth_col(Matrix* matrix,int nth){

    double* col = (double*) calloc(sizeof(double), matrix->n);

    for(int i=0; i<matrix->n; ++i)
        col[i] = matrix->matrix[i][nth];

    return col;
}

double multiply_row_column(int n, double* row, double* col){

    double sum = 0;

    for(int i=0; i<n; ++i)
        sum += row[i] * col[i];

    return sum;
}

void iter_over_matrix(Matrix* matrix, double row_index, double col_index, double* _real, double* _imag){

    if(matrix->matrix == NULL || matrix == NULL){
        fprintf(stdout, "matrix->matrix is NULL\n");
        exit(1);
    }

    double power, real, imag;

    for(int i=0; i<matrix->n;++i){
        for(int j=0; j<matrix->n; ++j){

            power = 2 * PI * (((row_index / matrix->n) * i) + ((col_index / matrix->n) * j));
            
            real = cos(power);
            imag = -sin(power);

            *_real += matrix->matrix[i][j] * real;
            *_imag += matrix->matrix[i][j] * imag;
        }
    }

}


void multiply_matrix(int s, int e){

    double *row, *col;

    for(int i=s; i<e; ++i){
        for(int j=0; j<matrix_A.n; ++j){
            
            row = get_nth_row(&matrix_A, i);
            col = get_nth_col(&matrix_B, j);

            matrix_C.matrix[i][j] = multiply_row_column(matrix_A.n, row, col);

            free(row);
            free(col);
        }
    }

}

void discrete_2d_fourier_transform(_thread_resp* resp, int s, int e){

    double real = 0, imag = 0;

    for(int row_index=0; row_index<e - s; ++row_index){
        for(int col_index=0; col_index<matrix_C.n; ++col_index){

            real = imag = 0;
            iter_over_matrix(&matrix_C,(double) (row_index + s), (double) col_index, &real, &imag);
            
            resp->t_real_matrix[row_index][col_index] = real;
            resp->t_imag_matrix[row_index][col_index] = imag;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void* thread_start(void* arg){

    thread_args* args = (thread_args*) arg;
    char current_time_str[150] = {0};
    struct timespec start, finish;
    double elapsed;

    clock_gettime(1, &start);

    _thread_resp *resp = (_thread_resp*) calloc(sizeof(_thread_resp), 1);

    init_response(resp, args->start, args->end, matrix_A.n);

    multiply_matrix(args->start, args->end);

    clock_gettime(1, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    set_current_time(current_time_str);
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Thread %d has reached the rendezvous point in %lf seconds.\n", args->id, elapsed);
    fflush(stdout);
    
    ////////////
    // sleep(2);

    if(sig_exit){
        free(arg);
        return resp;
    }

    pthread_mutex_lock(&mutex_task_1);
    task_1_done++;
    if(task_1_done < m){
        if(sig_exit){
            free(arg);
            return resp;
        }   
        pthread_cond_wait(&cond_task_1, &mutex_task_1);
    }
    else{
        pthread_cond_broadcast(&cond_task_1);
    }
    pthread_mutex_unlock(&mutex_task_1);

    // //////////
    // sleep(2);

    if(sig_exit){
        free(arg);
        return resp;
    }

    sched_yield();

    set_current_time(current_time_str);
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Thread %d is advancing to the second part\n", args->id);
    fflush(stdout);
    
    clock_gettime(1, &start);

    ////////////
    // sleep(2);

    discrete_2d_fourier_transform(resp, args->start, args->end);

    if(sig_exit){
        free(arg);
        return resp;
    }

    clock_gettime(1, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    set_current_time(current_time_str);
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "%s Thread %d has has finished the second part in %lf seconds.\n",current_time_str ,args->id, elapsed);
    fflush(stdout);

    free(arg);

    return (void*) resp;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){

    if(argc != 11){
        fprintf(stdout, "Usage: ./<executable_file> -i <filePath1> -j <filePath2> -o <output> -n <square_matrix_row_size> -m <thread_number>\n");
        exit(1);
    }

    n = atoi(argv[8]);
    m = atoi(argv[10]);

    if(n < 2){
        fprintf(stdout, "Error: n must be greater than 2\n");
        exit(1);
    }
    if(m % 2 != 0){
        fprintf(stdout, "Error: m must be even\n");
        exit(1);
    }

    sigset_t set_SIGINT, prev;
    sigemptyset(&set_SIGINT);
    sigaddset(&set_SIGINT, SIGINT);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &SIGINT_handler;
    if((sigaction(SIGINT, &sa, NULL) == -1)){
        perror("Sigaction wont work for SIGINT");
    }
    
    struct timespec start, finish;
    double elapsed;

    char* input_file_path_1 = argv[2];
    char* input_file_path_2 = argv[4];
    char* output_path = argv[6];

    int length = pow(pow(2, n), 2);
    char* content = (char*) calloc(sizeof(char*), length);
    
    init_matrix(&matrix_A, pow(2, n));
    read_file(input_file_path_1, length, content);
    place_the_matrix(&matrix_A, pow(2, n), content);

    /////////////
    // sleep(10);
    
    if(sig_exit){
        free_matrix(&matrix_A);
        free(content);
        exit(0);
    }
    
    init_matrix(&matrix_B, pow(2, n));
    read_file(input_file_path_2, length, content);
    place_the_matrix(&matrix_B,  pow(2, n), content);
    
    /////////////
    // sleep(10);

    free(content);
    if(sig_exit){
        free_matrix(&matrix_A);
        free_matrix(&matrix_B);
        exit(0);
    }

    clock_gettime(1, &start);

    pthread_t threads[m];
    pthread_mutex_init(&mutex_task_1, NULL);
    pthread_cond_init(&cond_task_1, NULL);

    /////////////
    // sleep(10);

    init_matrix(&matrix_C, pow(2, n));
    Matrix result_img_matrix, result_real_matrix;
    result_img_matrix.matrix = result_real_matrix.matrix = NULL;
    init_matrix(&result_img_matrix, pow(2, n));
    init_matrix(&result_real_matrix, pow(2, n));
    result_img_matrix.n = result_real_matrix.n = pow(2, n);

    if(sig_exit){
        free_matrix(&matrix_A);
        free_matrix(&matrix_B);
        free_matrix(&matrix_C);
        free_matrix(&result_img_matrix);
        free_matrix(&result_real_matrix);

        pthread_mutex_destroy(&mutex_task_1);
        pthread_cond_destroy(&cond_task_1);
        
        exit(0);
    }


    if(sigprocmask(0, &set_SIGINT, &prev))
        perror("Signal mask");
    for(int i=0; i<m; ++i){

        int start = i * (pow(2, n) / m);
        int end = (i+1) * (pow(2, n) / m);

        fprintf(stdout, "Thread %d will start from %d to %d\n", i, start, end);

        thread_args *args = (thread_args*) calloc(sizeof(thread_args), 1);
        args->start = start;    args->end = end;
        args->id = i;

        pthread_create(&threads[i], NULL, (void*) thread_start, args);
    }
    if(sigprocmask(2, &prev, NULL))
        perror("Signal mask");


    if(sig_exit){

        for(int i=0; i<m; ++i){
            _thread_resp* resp = NULL;
            pthread_join(threads[i], (void**) &resp);
            free_response(resp);
        }

        pthread_mutex_destroy(&mutex_task_1);
        pthread_cond_destroy(&cond_task_1);

        free_matrix(&matrix_A);
        free_matrix(&matrix_B);
        free_matrix(&matrix_C);
        free_matrix(&result_img_matrix);
        free_matrix(&result_real_matrix);

        exit(0);
    }
    
    /////////////
    // sleep(10);

    
    int i;
    for(i=0; i<m; ++i){

        _thread_resp *resp = NULL;
        pthread_join(threads[i], (void**) &resp);

        for(int i=resp->start, z = 0; i<resp->end; ++i, ++z){
            for(int j=0; j<matrix_C.n; ++j){
                result_img_matrix.matrix[i][j] = resp->t_imag_matrix[z][j];
                result_real_matrix.matrix[i][j] = resp->t_real_matrix[z][j];
            }
        }
        if(resp != NULL){
            free_response(resp);
        }
    }

    /////////////
    // sleep(10);

    if(sig_exit){

        fprintf(stdout, "SIGINT received\n");

        for(; i<m; ++i){
            _thread_resp* resp = NULL;
            pthread_join(threads[i], (void**)&resp);
            free_response(resp);
        }

        pthread_mutex_destroy(&mutex_task_1);
        pthread_cond_destroy(&cond_task_1);

        free_matrix(&matrix_A);
        free_matrix(&matrix_B);
        free_matrix(&matrix_C);
        free_matrix(&result_img_matrix);
        free_matrix(&result_real_matrix);

        exit(0);
    }
    
    /////////////
    // sleep(10);

    write_file(output_path, &result_real_matrix, &result_img_matrix);
    
    /////////////
    // sleep(10);

    if(sig_exit){
        pthread_mutex_destroy(&mutex_task_1);
        pthread_cond_destroy(&cond_task_1);

        free_matrix(&matrix_A);
        free_matrix(&matrix_B);
        free_matrix(&matrix_C);
        free_matrix(&result_img_matrix);
        free_matrix(&result_real_matrix);
        exit(0);
    }

    clock_gettime(1, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    fprintf(stdout, "The process has written the output file. The total time spent is %lf seconds.\n", elapsed);
    fflush(stdout);

    pthread_mutex_destroy(&mutex_task_1);
    pthread_cond_destroy(&cond_task_1);

    free_matrix(&matrix_A);
    free_matrix(&matrix_B);
    free_matrix(&matrix_C);
    free_matrix(&result_real_matrix);
    free_matrix(&result_img_matrix);

    return 0;
}
