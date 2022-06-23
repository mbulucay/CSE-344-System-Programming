/*
*    Muhammed Bedir ULUCAY
*    CSE-344 System Programming
*                  18.03.2022
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/file.h>
#include "lib.h"

#define MAX_KEY_VALUE_PAIR_SIZE 20


int main(int argc, char* argv[]){

    if(argc != 3){  
        perror("All argument not statisfied\n");
        return -1;
    }

    char* returnText = NULL;
    int count = 0, isSuccesful = -1;
    
    ReplaceWord pairs[MAX_KEY_VALUE_PAIR_SIZE];

    count = get_key_value_pairs(pairs, MAX_KEY_VALUE_PAIR_SIZE, argv[1]);

    returnText = read_from_file(argv[2], pairs, count);

    fflush(stdout);
    
    isSuccesful = write_to_file(argv[2], returnText);

    if(isSuccesful != 0)
        perror("Write to file failed\n");
    
    free(returnText);
    
    for(int i=0; i<count; ++i){
        free(pairs[i].oldWord);
        free(pairs[i].newWord);
    }

    return 0;
}