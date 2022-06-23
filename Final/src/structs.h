#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define MAX_SERVANT_NUM 100
#define INITIAL_SERVANT_THREAD_NUM 10000

#define REAL_ESTATE_SIZE 128
#define REAL_ESTATE_TRANSACTION_SIZE 128
#define DATE_FILE_SIZE 128
#define CITY_NAME_SIZE 32
#define DATE_SIZE 32
#define IP_SIZE 16

#define MAX_REQ_NUM 1024
#define MAX_LINE_BYTE 160
#define WORD_LENGTH 128

#define MAX_FOLDER 512
#define FOLDER_NAME_LEN 256
#define PATH_LEN 1024

#define BACKLOG 100

#define CITY_SIZE 200

typedef struct date{
    
    int day, month, year;

}date;


typedef struct server_request
{
    int type;

    int port_number, process_id, 
        lower_bound, upper_bound;
    char lower_city_name[CITY_NAME_SIZE];
    char upper_city_name[CITY_NAME_SIZE];
    char ip_address[IP_SIZE];

    char order_type[WORD_LENGTH];
    char real_estate[REAL_ESTATE_SIZE];
    date start_date, end_date;
    char city_name[CITY_NAME_SIZE];

}server_request;


typedef struct server_2_servant{

    date start_date, end_date;
    char real_estate[REAL_ESTATE_SIZE];
    char city_name[CITY_NAME_SIZE];

}server_2_servant;


typedef struct server_2_client{

    int val;

}server_2_client;


typedef struct client_thread_arg{

    int port_number;
    char ip_address[IP_SIZE];
    int nth;

    server_request request;    

}client_thread_arg;


typedef struct servant{

    int servant_fd, port_number,
        lower_bound, upper_bound;
    int process_id;
    
    char ip_address[IP_SIZE];
    char lower_city_name[CITY_NAME_SIZE];
    char upper_city_name[CITY_NAME_SIZE];

}servant;


typedef struct transaction_arg{

    int transaction_id;
    char real_estate_type[REAL_ESTATE_SIZE];
    char city_name[CITY_NAME_SIZE];
    int surface_area;
    unsigned long price;

}transaction_arg;


typedef struct date_file{

    date date;
    transaction_arg transaction[REAL_ESTATE_TRANSACTION_SIZE];
    int transaction_count;

}date_file;


typedef struct City{

    char city_name[CITY_NAME_SIZE];
    date_file date_file[DATE_FILE_SIZE];
    int date_file_count;

}City;


typedef struct folder{

    char name[FOLDER_NAME_LEN];
    char sub_names[MAX_FOLDER][FOLDER_NAME_LEN];

}folder;


void print_request(server_request req);

void print_date(date d);
void get_date(char str[DATE_SIZE], date* d);

void print_transaction_arg(transaction_arg t);

void print_date_file(date_file df);

void swap_file(char file_1[FOLDER_NAME_LEN], char file_2[FOLDER_NAME_LEN]);
void sort_folders(char directory[MAX_FOLDER][FOLDER_NAME_LEN], int num_folders);
void print_folders(folder resp_dir[], int num_folders);


void print_city(City c);
void print_cities(City cities[], int num_cities);

#endif // !STRUCTS_H
