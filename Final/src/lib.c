#include "lib.h"
#include "structs.h"


/*
    Socket functions for opening and closing as server and client
*/

int setup_server(int port, int backlog){

    struct sockaddr_in server_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0){
        perror("socket");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        exit(1);
    }

    if(listen(sockfd, backlog) < 0){
        perror("listen");
        exit(1);
    }

    return sockfd;
}

int setup_client(char *ip, int port, pthread_mutex_t *mutex){

    struct sockaddr_in client_addr;

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0){
        perror("socket");
        exit(1);
    }

    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(ip);
    client_addr.sin_port = htons(port);

    while(connect(client_fd , (struct sockaddr *)&client_addr , sizeof(client_addr)) == -1){
        close(client_fd);
        client_fd = socket(AF_INET , SOCK_STREAM , 0);
    }

    return client_fd;

}

int get_pid(){

    int pid = -1;
    char pidStr[100] = {0};

    int fd = open("/proc/self/stat", O_RDONLY);
    if(fd == -1){
        perror("open");
        return -1;
    }

    int read_count = read(fd,pidStr, 100);
    if(read_count == -1){
        perror("read");
        return -1;
    }    
    close(fd);
    
    sscanf(pidStr, "%d", &pid);

    return pid;
}


/*
    String Manipulation
*/

char** split_str(char* str, char* delim, int* split_count){

    char** result = (char**)calloc(sizeof(char*), 5);

    for(int i = 0; i < 5; ++i){
        result[i] = (char*)calloc(sizeof(char), WORD_LENGTH);
    }

    int len = strlen(str);
    int i = 0 , j = 0, k = 0;
    
    while(i < len){
        if(str[i] == delim[0]){
            result[j][k] = '\0';
            j++;
            k = 0;
            (*split_count)++;
            while(str[i] == delim[0]){
                i++;
            }
            i--;
        }
        else{
            result[j][k] = str[i];
            k++;
        }
        i++;

    }


    return result;
}


/*
    Data searching functions
*/

int in_between_date(date start, date end, date comp){

    if(comp.year < start.year || comp.year > end.year)
        return 0;

    if(comp.year == start.year && comp.month < start.month)
        return 0;

    if(comp.year == end.year && comp.month > end.month)
        return 0;

    if(comp.year == start.year && comp.month == start.month && comp.day < start.day)
        return 0;
    
    if(comp.year == end.year && comp.month == end.month && comp.day > end.day)
        return 0;

    return 1;
}

int compare_transaction_arg(transaction_arg t1, server_2_servant s2s){

    if(strcmp(t1.real_estate_type, s2s.real_estate) == 0){
        // fprintf(stdout, "real_estate_type: %s\n", t1.real_estate_type);
        return 1;
    }
    return 0;

}

int calculate_transaction_in_date_file(date_file df, server_2_servant s2s){

    int count = 0;

    for(int i = 0; i < df.transaction_count; i++){
        if(compare_transaction_arg(df.transaction[i], s2s)){
            // fprintf(stdout, "id: %d\n", df.transaction[i].transaction_id);
            count++;
        }
    }

    return count;
}

int calculate_transaction_in_city(City c, server_2_servant req){

    int count = 0;

    for(int i = 0; i < c.date_file_count; i++){
        if(in_between_date(req.start_date, req.end_date, c.date_file[i].date)){
            // fprintf(stderr, "in between date\n");
            count += calculate_transaction_in_date_file(c.date_file[i], req);
        }
    }

    return count;

}

int calculate_transaction(City cities[], int len, server_2_servant req){

    int count = 0;

    for(int i = 0; i < len; i++){
        if(strcmp(req.city_name, "all") == 0 || strcmp(req.city_name, cities[i].city_name) == 0){
            // fprintf(stderr, "calculate_transaction: %s\n", cities[i].city_name);
            count += calculate_transaction_in_city(cities[i], req);
        }
    }

    return count;

}

