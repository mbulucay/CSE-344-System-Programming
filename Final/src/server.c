#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#include "structs.h"
#include "queue.h"
#include "lib.h"

pthread_mutex_t request_mutex;
pthread_mutex_t connect_mutex;
pthread_mutex_t global_mutex;

pthread_cond_t request_cond;

node *head = NULL;
pthread_t *threads = NULL;

servant servants[MAX_SERVANT_NUM];
char current_time_str[WORD_LENGTH] = { 0 };
int servant_num = 0;
int port_number = 0;
int num_threads = 0;
int client_counter = 0;
int servant_counter = 0;

sig_atomic_t sigint_flag = 0;

// thread pool main function
void* thread_main(void* arg);

// handling requests funcitons
void handle_request(int req_fd);
void handle_client(server_request req, int req_fd);
void response_with_multi_city(server_request req, int req_fd);

// servant connection and finding servant functions
void create_servant_connection(server_request req);
servant get_servant_with_names(char city_name[CITY_NAME_SIZE]);

// program requirement functions
void signal_handler();
void SIGINT_handler(int signo);
void set_current_time();


int main(int argc, char* argv[]){

    if(argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-t") != 0){
        fprintf(stderr, "usage: %s -p <PORT> -t <number_of_threads> ", argv[0]);        
        exit(1);
    }

    port_number = atoi(argv[2]);
    num_threads = atoi(argv[4]);

    if(num_threads < 5){
        fprintf(stderr, "Number of threads must be at least 5");
        exit(1);
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &SIGINT_handler;
    if((sigaction(SIGINT, &sa, NULL) == -1))
        perror("Sigaction");

    if(pthread_mutex_init(&request_mutex, NULL) != 0 || pthread_mutex_init(&connect_mutex, NULL) != 0 || pthread_mutex_init(&global_mutex, NULL) != 0){
        fprintf(stderr, "mutex init failed");
        exit(1);
    }

    if(pthread_cond_init(&request_cond, NULL) != 0){
        fprintf(stderr, "cond init failed");
        exit(1);
    }

    threads = (pthread_t*)calloc(sizeof(pthread_t), num_threads);
    for(int i = 0; i < num_threads; ++i){
        pthread_create(&threads[i], NULL, (void*) thread_main, head);
    }

    int server_fd = setup_server(port_number, BACKLOG);

    struct sockaddr client;
    socklen_t addrlen = sizeof(client);

    for(;;){

        int req_fd = accept(server_fd, &client, &addrlen);
        if(req_fd < 0 && !sigint_flag){
            fprintf(stderr, "accept error\n");
            fflush(stderr);
            exit(1);
        }

        if(sigint_flag){
            signal_handler();
            exit(0);
        }

        int* req_fd_ptr = (int*) calloc(sizeof(int), 1);
        *req_fd_ptr = req_fd;
        
        pthread_mutex_lock(&request_mutex);
        
        enqueue(&head, req_fd_ptr);
        pthread_cond_signal(&request_cond);

        pthread_mutex_unlock(&request_mutex);

    }

    return 0;
}


void* thread_main(void* arg){

    for(;;){
        
        int* req_fd_ptr;
        
        pthread_mutex_lock(&request_mutex);

        req_fd_ptr = dequeue(&head);
        if(req_fd_ptr == NULL){
            pthread_cond_wait(&request_cond, &request_mutex);
            req_fd_ptr = dequeue(&head);
        }
        
        pthread_mutex_unlock(&request_mutex);

        if(sigint_flag){
            if(req_fd_ptr != NULL)
                free(req_fd_ptr);
            return NULL;           
        }

        if(req_fd_ptr != NULL){
            handle_request(*req_fd_ptr);
            free(req_fd_ptr);
        }
    }

    return NULL;
}


void handle_request(int req_fd){

    server_request req;
    memset(&req, 0, sizeof(server_request));
    recv(req_fd, &req, sizeof(server_request), 0);

    pthread_mutex_lock(&global_mutex);
    (req.type == 0) ? servant_counter++ : client_counter++;
    pthread_mutex_unlock(&global_mutex);

    if(req.type == 0){
        create_servant_connection(req);
    }
    else if(req.type == 1){
        handle_client(req, req_fd);
    }
}


void create_servant_connection(server_request req){

    servant s;
    s.port_number = req.port_number;
    s.lower_bound = req.lower_bound;
    s.upper_bound = req.upper_bound;
    s.process_id = req.process_id;
    strcpy(s.lower_city_name, req.lower_city_name);
    strcpy(s.upper_city_name, req.upper_city_name);
    strcpy(s.ip_address, req.ip_address);

    set_current_time();
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Servant %d present at port %d handling cities %s-%s\n", s.process_id, s.port_number, s.lower_city_name, s.upper_city_name);

    pthread_mutex_lock(&global_mutex);
    servants[servant_num++] = s;
    pthread_mutex_unlock(&global_mutex);

}


void handle_client(server_request req, int req_fd){

    server_2_client res;
    server_2_servant req_servant;

    set_current_time();
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Request arrived \"%s %s %d-%d-%d  %d-%d-%d %s\"\n", req.order_type, req.real_estate, req.start_date.day, req.start_date.month, req.start_date.year, req.end_date.day, req.end_date.month, req.end_date.year, req.city_name);

    if(servant_num == 0){
        // fprintf(stderr, "There is no servant in server\n");
        res.val = -1;
        send(req_fd, &res, sizeof(server_2_client), 0);
    }

    if(strcmp(req.city_name, "all") == 0){
        response_with_multi_city(req, req_fd);
        return;
    }

    servant servant = get_servant_with_names(req.city_name);

    if(servant.port_number == -1){
        // fprintf(stderr, "There is no servant in server\n");
        res.val = -1;
        send(req_fd, &res, sizeof(server_2_client), 0);
        return;
    }

    set_current_time();
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Contacting servant %d at port %d\n", servant.process_id, servant.port_number);

    strcpy(req_servant.city_name, req.city_name);
    req_servant.start_date = req.start_date;
    req_servant.end_date = req.end_date;
    strcpy(req_servant.real_estate, req.real_estate);

    int servant_fd = setup_client(servant.ip_address, servant.port_number, &connect_mutex);

    if(send(servant_fd, &req_servant, sizeof(server_2_servant), 0) < 0){
        fprintf(stderr, "send error 2\n"); fflush(stderr);
        exit(1);
    }

    if(recv(servant_fd, &res.val, sizeof(int), 0) < 0){
        fprintf(stderr, "recv error 2\n"); fflush(stderr);
        exit(1);
    }

    if(send(req_fd, &res, sizeof(server_2_client), 0) < 0){
        fprintf(stderr, "send error 3\n"); fflush(stderr);
        exit(1);
    }

    set_current_time();
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Response received: %d, forwarded to client\n", res.val);

}


void response_with_multi_city(server_request req, int req_fd){

    int sum = 0;
    int val = 0;
    server_2_client res;
    server_2_servant req_servant;

    set_current_time();
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Contacting ALL servants\n");

    req_servant.start_date = req.start_date;
    req_servant.end_date = req.end_date;
    strcpy(req_servant.city_name, req.city_name);
    strcpy(req_servant.real_estate, req.real_estate);

    for(int i = 0; i < servant_num; ++i){

        int sockfd = setup_client(servants[i].ip_address, servants[i].port_number, &connect_mutex);

        if(send(sockfd, &req_servant, sizeof(server_2_servant), 0) < 0){
            fprintf(stderr, "send error 1\n"); fflush(stderr);
            exit(1);
        }

        if(recv(sockfd, &val, sizeof(int), 0) < 0){
            fprintf(stderr, "recv error 1\n"); fflush(stderr);
            exit(1);
        }

        close(sockfd);
        sum += val;
    }

    res.val = sum;

    if(send(req_fd, &res, sizeof(server_2_client), 0) < 0){
        fprintf(stderr, "send error 2\n"); fflush(stderr);
        exit(1);
    }

    set_current_time();
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "Response received: %d, forwarded to client\n", sum);

}


void signal_handler(){

    set_current_time();
    fprintf(stdout, "%s ", current_time_str);
    fprintf(stdout, "SIGINT has been received. I handled a total of %d requests. Goodbye.\n", client_counter);

    while(1){
        int* a = dequeue(&head);
        if(a == NULL)
            break;
        free(a);
    }

    for(int i=0; i<servant_num; ++i){
        kill(servants[i].process_id, SIGINT);
    }

    for(int i=0; i<num_threads; ++i){
        pthread_join(threads[i], NULL);
    }

    if(threads != NULL)
        free(threads);

    pthread_mutex_destroy(&request_mutex);
    pthread_mutex_destroy(&connect_mutex);
    pthread_mutex_destroy(&global_mutex);
    pthread_cond_destroy(&request_cond);

}

void SIGINT_handler(int signo){
    
    if(signo == SIGINT){
        sigint_flag = 1;
        pthread_cond_broadcast(&request_cond);
    }

}

void set_current_time(){

    int milli_sec;
    char str_time[100];
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    milli_sec = ( current_time.tv_usec / 1000 );

    strftime(str_time, 100, "%H:%M:%S", localtime(&current_time.tv_sec));
    sprintf(current_time_str, "%s:%03d", str_time, milli_sec);
}


servant get_servant_with_names(char city_name[CITY_NAME_SIZE]){

    for(int i = 0; i < servant_num; ++i){
        if(strcmp(servants[i].lower_city_name, city_name) <= 0 && strcmp(servants[i].upper_city_name, city_name) >= 0){
            return servants[i];
        }
    }
    servant s;
    s.port_number = -1;
    return s;
}
