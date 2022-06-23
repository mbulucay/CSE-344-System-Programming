#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#include "syscall_wrappers.h"
#include "vector.h"

extern int errno;

#define NO_EINTR(stmt) while (((stmt) == -1) && errno == EINTR )

#define WRITE_FLAG (O_CREAT | O_WRONLY | O_APPEND)
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define READ_FLAGS (O_RDONLY)

#define VECTOR_SIZE 3
#define POINT_NUMBER 10
#define NUM_STR_LENGTH 20

sig_atomic_t is_get = 0;
sig_atomic_t deleted_child = 0;

int *child_processIds = NULL;
int child_processIds_size = 0;
int output_fd = -1;
int input_fd = -1;

char* inputFile = NULL;
char* outputFile = NULL;
char* num = NULL;
Matrix* m = NULL;


void SIGINT_handler(int signo);
void check_SIGINT();

double calculate_Frobenius(Matrix _matrix);
double closest_Matrices(Matrix *matrix, int size, int* index1, int* index2);

int read_file();

void read_matrices(Matrix *_matrix);
void print_matrices(double min, int index1, int index2);

void print_points(char* points);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){

    if(argc < 5){
        perror("All arguments not satisfied");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &SIGINT_handler;
    if((sigaction(SIGINT, &sa, NULL) == -1))
        perror("Sigaction");
    
    child_processIds = (int*) w_calloc (0, sizeof(int));
    m = (Matrix*) w_calloc(0, sizeof(Matrix));

    inputFile = argv[2];
    outputFile = argv[4];
    
    check_SIGINT();
    input_fd = w_open(inputFile, READ_FLAGS);
    
    check_SIGINT();
    int counter = read_file();

    if(child_processIds_size == 0){
        free(m);
        free(child_processIds);
        return 0;
    }
    m = (Matrix*) w_realloc(m, counter * sizeof(Matrix));
    
    check_SIGINT();
    read_matrices(m);
    sleep(10);
    int index1, index2;
    double min = closest_Matrices(m, counter, &index1, &index2);
    
    check_SIGINT();
    print_matrices(min, index1, index2);
    
    free(m);
    free(child_processIds);

    return 0;
}


void print_points(char* points){

    fprintf(stdout, "Created R_%d with ", child_processIds_size);
    for(int i=0; i<POINT_NUMBER * VECTOR_SIZE; i+=3)
        fprintf(stdout, "(%d, %d, %d)",points[i], points[i+1], points[i+2]);

    fprintf(stdout,"\n");
}   


/*
    For cleaning memory, closing files and deleting child process
*/
void check_SIGINT(){
    
    if(is_get == 0) return;

    if(input_fd != -1)
        w_close(input_fd);
    if(output_fd != -1)
        w_close(output_fd);


    int pid = fork();
    if(pid == 0){
        char* argv[] = {
            "rm", outputFile, NULL
        };

        execv("/bin/rm", argv);

        perror("execv in check_SIGINT");
        exit(EXIT_FAILURE);
    }
    
    // cleaning rm child process and if any not cleaned child process
    for(;;){
        int child_pid = wait(NULL);
        if(child_pid == -1 && errno == ECHILD){
            fprintf(stdout, "All children cleaned\n");
            break;
        }
        kill(child_pid, SIGINT);
    }
    fprintf(stdout, "All children cleaned\n");

    //Relasing the heap memory
    if(m != NULL)   
        free(m);
    if(child_processIds != NULL)
        free(child_processIds);
    if(num != NULL)
        free(num);

    fprintf(stdout, "Memory cleaned\n");
    fflush(stdout);
    
    exit(EXIT_SUCCESS);
}


/*
    Calculating the Frobenius norm of a matrix
*/
double calculate_Frobenius(Matrix _matrix){

    double sum = 0.0;

    for(int i=0; i<VECTOR_SIZE; ++i)
        for(int j=0; j<VECTOR_SIZE; ++j)
            sum += pow(_matrix.matrix[i][j], 2);

    return sqrt(sum);
}


/*
    Searching the closest 2 matrix using the Frobenious norm of matrices
*/
double closest_Matrices(Matrix *matrix, int size, int* index1, int* index2){

    double min = 9999999999999.999;
    double temp = 0.0; 

    for(int i = 0; i < size; ++i){
        for(int j = i; j < size; ++j){

            if(i == j) continue;
            
            temp = fabs(calculate_Frobenius(matrix[i]) - calculate_Frobenius(matrix[j]));
            // fprintf(stdout,"dif betwwen  (%d,%d) : %lf\n", i, j, temp);
            if(temp < min){
                min = temp;
                *index1 = i;
                *index2 = j;
            }
        }
    } 

    return min;
}


/*
    Reading the matrices from result file
*/
void read_matrices( Matrix *_matrix){

    num = w_calloc(NUM_STR_LENGTH, sizeof(char));

    output_fd = w_open(outputFile, READ_FLAGS);

    struct flock lock;  
    memset(&lock , 0 ,sizeof(lock));
    lock.l_type = F_RDLCK;

    if(fcntl(output_fd,F_SETLKW,&lock) == -1)
        perror("fcntl");


    int index = 0, m_index = 0;
    int mat_loc = 0; 
    
    for( ; ; ){

        char ch;
        int read_bytes = 0;
        while (((read_bytes = read(output_fd, &ch, sizeof(char))) == -1) 
                && (errno == EINTR)){
            continue;
        }

        if(read_bytes == 0)
            break;

        if(ch == '\n'){
            mat_loc = 0;
            m_index++;
            continue;
        }
       
        if(ch == ','){
            double val;  
            sscanf(num, "%lf", &val);
            _matrix[m_index].matrix[mat_loc / 3][mat_loc % 3] = val;
            index = 0;
            mat_loc++;

        }else if(ch != '\n'){
            num[index] = ch;
            index++;
        }
    }

    lock.l_type =F_UNLCK;
    if(fcntl(output_fd,F_SETLKW,&lock) == -1)
        perror("fcntl");

    w_close(output_fd);

    free(num);
}


/*
    SIGINT Handler function''
*/
void SIGINT_handler(int signo){
    is_get = 1;
}


/*
    Printing the result matrices
*/
void print_matrices(double min, int index1, int index2){

    fprintf(stdout, "Min distance :%lf\n", min);

    fprintf(stdout, "Matrix_1: R_%d\n", index1);
    for(int i = 0; i < VECTOR_SIZE; ++i){
        for(int j = 0; j < VECTOR_SIZE; ++j){
            fprintf(stdout," %.3lf", m[index1].matrix[i][j]);
        }
        fprintf(stdout,"\n");
    }

    fprintf(stdout, "Matrix_2: R_%d\n", index2);
    for(int i = 0; i < VECTOR_SIZE; ++i){
        for(int j = 0; j < VECTOR_SIZE; ++j){
            fprintf(stdout," %.3lf", m[index2].matrix[i][j]);
        }
        fprintf(stdout,"\n");
    }
}


/*
    Reading the input file and after each 30 bytes creating a new child process
    for the calculation 
*/
int read_file(){
    
    int counter = 0,
        point_counter = 0; 

    char points[POINT_NUMBER * VECTOR_SIZE] = {0};
    char buffer[VECTOR_SIZE];

    for( ; ; ){
        int read_bytes = 0;
        while (((read_bytes = read(input_fd, buffer, sizeof(buffer))) == -1) 
                && (errno == EINTR)){
            continue;
        }

        check_SIGINT();
        if (read_bytes < 3)
            break;

        if(read_bytes == 3){
            points[point_counter * 3] = buffer[0];
            points[point_counter * 3 + 1] = buffer[1];
            points[point_counter * 3 + 2] = buffer[2];
            point_counter++;
        }else{
            lseek(input_fd, -read_bytes, SEEK_CUR);
        }

        if(point_counter == 10){
            check_SIGINT();

            char* argv[] = { 
                outputFile, NULL
            };

            char* env[] = { 
                points,
                NULL
            };

            print_points(points);
            int pid = fork();
            counter++;
            if(pid == 0){
                int f = execve("./ch", argv, env);
                perror("execvp Error");
                exit(EXIT_FAILURE);
            }

            check_SIGINT();
            
            child_processIds = (int*) w_realloc (child_processIds, (++child_processIds_size) * sizeof(int));
            child_processIds[child_processIds_size - 1] = pid;
            
            for(int i = 0; i< POINT_NUMBER * VECTOR_SIZE ; ++i)
                points[i] = '\0';

            point_counter = 0;
        }
        check_SIGINT();
    } 

    for(;;){
        int child_pid = wait(NULL);
        deleted_child++;
        if(child_pid == -1){
            if(errno == ECHILD){
                return counter;
            }
        }
    }

    return counter;
}

