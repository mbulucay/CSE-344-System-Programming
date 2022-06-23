#ifndef MYLIB_H
#define MYLIB_H

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <strings.h>

#include "structs.h"

/*
    Socket functions for opening and closing as server and client
*/

int setup_server(int port, int backlog);
int setup_client(char *ip, int port, pthread_mutex_t *mutex);

int get_pid();


/*
    String manipulation functions
*/
char** split_str(char* str, char* delim, int* split_count);


/*
    Data searching functions
*/
int in_between_date(date start, date end, date comp);
int compare_transaction_arg(transaction_arg t1, server_2_servant s2s);
int calculate_transaction_in_date_file(date_file df, server_2_servant s2s);
int calculate_transaction_in_city(City c, server_2_servant req);
int calculate_transaction(City cities[], int len, server_2_servant req);


#endif // !MYLIB_H

