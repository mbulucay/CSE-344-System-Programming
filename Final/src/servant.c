#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>

#include "structs.h"
#include "lib.h"

pthread_mutex_t mutex;
sig_atomic_t sigint_flag = 0;

char* directory_path = NULL;
int split_count = 0;
int unique_port;
int pid = -1;
int request_count = 0;

int responsible_directories_counter = 0;
City cities[CITY_SIZE];

pthread_t *threads;
int thread_size = INITIAL_SERVANT_THREAD_NUM;
int threads_counter = 0;

// Data reading and storing functions 
void get_sub_folders(folder resp_dir[], int num_folders, int* num_data);
void get_border(char* str, int* lower_bound, int* upper_bound);
void list_data_folders(char* path, char directory[MAX_FOLDER][FOLDER_NAME_LEN], int* folder_counter, int type);
void print_data_paths(folder resp_dir[], int num_folders, int num_data);
void get_data_from_dataset(folder resp_dir[], int number_of_resp_file, int number_of_transactions_file, City cities[]);

void parse_transaction_file(char* str, transaction_arg* t);
void read_data_file(char* file_path, transaction_arg data[], int* num_data);

// Arg functions
void set_folder_boundaries(char directory[MAX_FOLDER][FOLDER_NAME_LEN], folder resp_dir[], int* lower_bound, int* upper_bound, int* num_folders);

// Server sockets base operation functions 
void* handle_client(void* arg);
int setup_server_servant(int port, int backlog);
void signal_handler();
void SIGINT_handler(int signo);


int main(int argc, char* argv[]){

    if(argc != 9 || strcmp(argv[1], "-d") != 0 || strcmp(argv[3], "-c") != 0 || strcmp(argv[5], "-r") != 0 || strcmp(argv[7], "-p") != 0){
        fprintf(stderr, "usage: %s -d <directoryPath> -c <border numbers> -r <IP> -p <PORT> ", argv[0]);
        exit(1);
    }

    directory_path = argv[2];
    char* borders = argv[4];
    char* ip_address = argv[6];
    char* port_number = argv[8];

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &SIGINT_handler;
    if((sigaction(SIGINT, &sa, NULL) == -1))
        perror("Sigaction");


    int lower_bound, upper_bound, main_folder_counter = 0,
        num_data_file = 0;

    pid = get_pid();
    if(pid == -1){
        perror("get_pid");
        exit(1);
    }

    get_border(borders, &lower_bound, &upper_bound);
    
    char main_directory[MAX_FOLDER][FOLDER_NAME_LEN];
    folder responsible_directories[upper_bound - lower_bound + 1];

    list_data_folders(directory_path, main_directory, &main_folder_counter, DT_DIR);
    
    sort_folders(main_directory, main_folder_counter);
    set_folder_boundaries(main_directory, responsible_directories, &lower_bound, &upper_bound, &responsible_directories_counter);

    get_sub_folders(responsible_directories, responsible_directories_counter, &num_data_file);
    get_data_from_dataset(responsible_directories, responsible_directories_counter, num_data_file, cities);

    if(sigint_flag){
        signal_handler();
        exit(0);
    }

    int server_listen_fd = setup_server_servant(0, BACKLOG);

    int client_fd = setup_client(ip_address, atoi(port_number), &mutex);

    fprintf(stdout, "Servant %d: loaded dataset, cities %s-%s\n", pid, cities[0].city_name, cities[responsible_directories_counter - 1].city_name);
    fprintf(stdout, "Servant %d: listening on port %d\n", pid, unique_port);

    server_request req;
    req.port_number = unique_port;
    req.lower_bound = lower_bound;
    req.upper_bound = upper_bound;
    strcpy(req.ip_address, ip_address);
    strcpy(req.lower_city_name, cities[0].city_name); 
    strcpy(req.upper_city_name, cities[responsible_directories_counter - 1].city_name);
    req.process_id = pid;

    if(send(client_fd, &req, sizeof(server_request), 0) < 0){
        perror("send");
        exit(1);
    }
    close(client_fd);


    if(sigint_flag){
        signal_handler();
        exit(0);
    }

    threads = (pthread_t*) calloc(sizeof(pthread_t), thread_size);
    for(;;){

        // fprintf(stdout, "Servant is waiting for client... %d\n", server_listen_fd);

        int fd = accept(server_listen_fd, NULL, NULL);
        if(fd < 0 && !sigint_flag){
            perror("accept");
            exit(1);
        }
        if(sigint_flag == 1){
            fprintf(stdout, "Servant %d: termination message received, handled %d request in total\n", pid, request_count);
            signal_handler();
            break;
        }
        
        request_count++;
        int * client_fd_ptr = (int*) calloc(sizeof(int), 1);
        *client_fd_ptr = fd;
        pthread_create(&threads[threads_counter], NULL, handle_client, (void*)client_fd_ptr);
        threads_counter++;
    }

    return 0;
}

/*  Arg functions */

void set_folder_boundaries(char directory[MAX_FOLDER][FOLDER_NAME_LEN], folder resp_dir[], int* lower_bound, int* upper_bound, int* num_folders){

    int i;
    for(i = 0; i < *upper_bound - *lower_bound + 1; i++){
        strcpy(resp_dir[i].name, directory[i + *lower_bound - 1]);
    }

    *num_folders = *upper_bound - *lower_bound + 1;
}


/*  Data reading and storing functions  */

void get_sub_folders(folder resp_dir[], int num_folders, int* num_data){

    char path[PATH_LEN / 2];
    char path2[PATH_LEN];

    char sub_folders[MAX_FOLDER][FOLDER_NAME_LEN];
    int sub_folders_counter = 0;

    for(int i = 0; i < num_folders; i++){

        sprintf(path, "%s/%s", directory_path, resp_dir[i].name);
        
        list_data_folders(path, sub_folders, &sub_folders_counter, DT_REG);

        for(int j = 0; j < sub_folders_counter; j++){
            sprintf(path2, "%s/%s", resp_dir[i].name, sub_folders[j]);
            strcpy(resp_dir[i].sub_names[j], sub_folders[j]);
        }

        *num_data = sub_folders_counter;
        sub_folders_counter = 0;
    }

    return;
}

void get_border(char* str, int* lower_bound, int* upper_bound){

    int len = strlen(str);

    int i;
    for(i = 0; i < len; i++){
        if(str[i] == '-'){
            break;
        }
    }

    *lower_bound = atoi(str);
    *upper_bound = atoi(str + i + 1);
}

void list_data_folders(char* path, char directory[MAX_FOLDER][FOLDER_NAME_LEN], int* folder_counter, int type){

    DIR* dir = opendir(path);
    if(dir == NULL){
        perror("opendir");
        exit(1);
    }

    struct dirent* entry;
    int index = 0;
    while((entry = readdir(dir)) != NULL){
        if(entry->d_type == type){
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
                continue;
            }
            strcpy(directory[index], entry->d_name);
            index++;
        }
    }

    *folder_counter = index;

    closedir(dir);
}


/*  Data reading and storing functions  */

void print_data_paths(folder resp_dir[], int num_folders, int num_data){
    
    int i, j;
    for(i = 0; i < num_folders; i++){
        printf("[%d] > %s\n",i , resp_dir[i].name);
        for(j = 0; j < num_data; j++){
            printf("\t[%d] > %s\n",j , resp_dir[i].sub_names[j]);
        }
    }
    
}

void get_data_from_dataset(folder resp_dir[], int number_of_resp_file, int number_of_transactions_file, City cities[]){

    // fprintf(stdout, "number_of_resp_file: %d\n", number_of_resp_file);
    // fprintf(stdout, "number_of_transactions_file: %d\n", number_of_transactions_file);

    char path[PATH_LEN];
    int number_of_data_in_file;

    for(int i=0; i<number_of_resp_file; i++){

        for(int j=0; j<number_of_transactions_file; j++){
            
            sprintf(path, "%s/%s/%s", directory_path, resp_dir[i].name, resp_dir[i].sub_names[j]);

            read_data_file(path, cities[i].date_file[j].transaction, &number_of_data_in_file);
            cities[i].date_file[j].transaction_count = number_of_data_in_file;

            date d;
            get_date(resp_dir[i].sub_names[j], &d);
            cities[i].date_file[j].date = d;

            strcpy(cities[i].city_name, resp_dir[i].name);
            cities[i].date_file_count = number_of_transactions_file;

        }
    }

}

void parse_transaction_file(char* str, transaction_arg* t){

    char** result = split_str(str, " ", &split_count);

    t->transaction_id = atoi(result[0]);
    strcpy(t->real_estate_type, result[1]);
    strcpy(t->city_name, result[2]);
    t->price = atoi(result[3]);
    t->surface_area = atoi(result[4]);

    for(int i = 0; i < 5; i++){
        free(result[i]);
    }

    free(result);

    split_count = 0;

}

void read_data_file(char* file_path, transaction_arg data[], int* num_data){

    char buffer[MAX_LINE_BYTE];
    memset(buffer, 0, MAX_LINE_BYTE);

    int fd = open(file_path, O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(1);
    }

    char character;
    int index = 0, data_idx = 0, line_count = 0;

    for(;;){

        int n = read(fd, &character, 1);
        if(n == -1){ perror("read"); exit(1); }

        if(n == 0){
            break;
        }
        if(character == '\n'){
            
            parse_transaction_file(buffer, &data[data_idx]);
            data_idx++;

            memset(buffer, 0, MAX_LINE_BYTE);
            index = 0;
            line_count++;
        }
        else{
            buffer[index] = character;
            index++;
        }

    }

    *num_data = line_count;

    close(fd);

}


/*  Server sockets base operation functions     */

void signal_handler(){

    fprintf(stdout, "Servant %d: termination message received, handled %d request in total\n", pid, request_count);

    for(int i = 0; i < threads_counter; i++){
        pthread_join(threads[i], NULL);
    }
    if(threads != NULL)
        free(threads);
}

void SIGINT_handler(int signo){
    
    if(signo == SIGINT){
        sigint_flag = 1;
    }

}

void* handle_client(void* arg){

    int client_fd = *(int*)arg;
    free(arg);
    
    server_2_servant s2s;
    recv(client_fd, &s2s, sizeof(server_2_servant), 0);

    int result = calculate_transaction(cities, responsible_directories_counter, s2s);

    send(client_fd, &result, sizeof(int), 0);

    return NULL;
}

int setup_server_servant(int port, int backlog){

    struct sockaddr_in server_addr;

    int server_listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_listen_fd < 0){
        perror("socket");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = port;

    if(bind(server_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        exit(1);
    }

    socklen_t len = sizeof(server_addr);
    if(getsockname(server_listen_fd, (struct sockaddr *)&server_addr, &len) < 0){
        perror("getsockname");
        exit(1);
    }

    unique_port = ntohs(server_addr.sin_port);

    if(listen(server_listen_fd, backlog) < 0){
        perror("listen");
        exit(1);
    }

    return server_listen_fd;
}
