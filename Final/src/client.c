#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "structs.h"
#include "lib.h"

int req_num = 0; 
int split_count = 0;
pthread_mutex_t connect_mutex;
pthread_barrier_t barrier;

// Request file read functions
server_request parse_request(char* request_str);
void read_request_file(char* file_path, server_request req[]);

// thread main function
void* thread_main(void* arg);

// helper functions
void copy_local_arg(client_thread_arg* local_arg, client_thread_arg* arg);


int main(int argc, char* argv[]){

    if(argc != 7 || strcmp(argv[1], "-r") != 0 || strcmp(argv[3], "-q") != 0 || strcmp(argv[5], "-s") != 0){
        fprintf(stderr, "%s -r <requestFile> -q <PORT> -s <IP>", argv[0]);
        exit(1);
    }

    char* request_file = argv[2];
    char* port_number = argv[4];
    char* ip_address = argv[6];

    server_request req[MAX_REQ_NUM];

    if(pthread_mutex_init(&connect_mutex, NULL) != 0){
        fprintf(stderr, "pthread_mutex_init error\n");
        exit(1);
    }

    read_request_file(request_file, req);

    if(pthread_barrier_init(&barrier, NULL, req_num) != 0){
        fprintf(stderr, "pthread_barrier_init error\n");
        exit(1);
    }

    fprintf(stdout, "Client: I have loaded %d requests and I'm creating %d threads\n", req_num, req_num);

    pthread_t thread_id[MAX_REQ_NUM];
    for(int i =0 ;i <req_num; ++i){

        client_thread_arg* arg = (client_thread_arg*)calloc(sizeof(client_thread_arg), 1);

        int *nth = (int*)calloc(sizeof(int), 1);
        *nth = i;

        arg->nth = *nth;
        arg->request = req[i];
        arg->port_number = atoi(port_number);
        strcpy(arg->ip_address, ip_address);
    
        int ret = pthread_create(&thread_id[i], NULL, (void*) thread_main, (void*) arg);
        if(ret != 0){
            fprintf(stderr, "pthread_create error\n");
            fflush(stderr);
            exit(1);
        }

        free(nth);
    }

    for(int i = 0; i < req_num; ++i){
        pthread_join(thread_id[i], NULL);
    }

    fprintf(stdout, "Client: All threads have terminated, goodbye.\n");
    fflush(stdout);

    pthread_mutex_destroy(&connect_mutex);
    pthread_barrier_destroy(&barrier);

    return 0;
}


void* thread_main(void* _arg){

    client_thread_arg arg;
    int val;

    arg.request.type = 1;
    copy_local_arg(&arg, (client_thread_arg*)_arg);
    free(_arg);

    pthread_barrier_wait(&barrier);

    fprintf(stdout, "Client-Thread-%d: Thread-%d has been created\n", arg.nth, arg.nth);

    server_request req = arg.request;
    req.type = 1;
    int sockfd = setup_client(arg.ip_address, arg.port_number, &connect_mutex);
    
    fprintf(stdout, "Client-Thread-%d: I am requesting \" /%s %s %d-%d-%d  %d-%d-%d  %s\"\n", arg.nth, req.order_type, req.real_estate, req.start_date.day, req.start_date.month, req.start_date.year, req.end_date.day, req.end_date.month, req.end_date.year, req.city_name);
    send(sockfd, &req, sizeof(server_request), 0);

    recv(sockfd, &val, sizeof(int), 0);
    fprintf(stdout, "Client-Thread-%d: The server's response to \"%s %s %d-%d-%d  %d-%d-%d  %s\" is %d\n", arg.nth, req.order_type, req.real_estate, req.start_date.day, req.start_date.month, req.start_date.year, req.end_date.day, req.end_date.month, req.end_date.year, req.city_name, val);

    close(sockfd);

    return NULL;
}


void copy_local_arg(client_thread_arg* local_arg, client_thread_arg* arg){
    
    local_arg->request = arg->request;
    strcpy(local_arg->ip_address, arg->ip_address);
    local_arg->port_number = arg->port_number;
    local_arg->nth = arg->nth;

}

void read_request_file(char* file_path, server_request req[]){

    int fd = open(file_path, O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(1);
    }

    char buffer[MAX_LINE_BYTE];
    memset(buffer, 0, MAX_LINE_BYTE);

    char character;
    int index = 0;
    for(;;){

        int n = read(fd, &character, 1);
        if(n == -1){ perror("read"); exit(1); }

        if(n == 0){
            break;
        }
        if(character == '\n'){
            if(index == 0){
                continue;
            }
            req[req_num] = parse_request(buffer);
            req_num++;
            memset(buffer, 0, MAX_LINE_BYTE);
            index = 0;
        }
        else{
            buffer[index] = character;
            index++;
        }
    }
}


server_request parse_request(char* request_str){

    server_request req;
    char** words = split_str(request_str, " ", &split_count);
    
    strcpy(req.order_type, words[0]);
    strcpy(req.real_estate, words[1]);

    get_date(words[2], &req.start_date);
    get_date(words[3], &req.end_date);

    if(split_count == 4){
        strcpy(req.city_name, words[4]);
    }else{
        strcpy(req.city_name, "all");
    }

    for(int i = 0; i < 5; ++i){
        free(words[i]);
    }
    free(words);

    split_count = 0;

    return req;
}

