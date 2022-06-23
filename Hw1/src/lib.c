#include "lib.h"
#include "syscall_wrappers.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/file.h>

#define WRITE_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define READ_FLAGS (O_RDONLY)
#define PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static int FILE_BYTE_SIZE = 4096;
static int BUF_SIZE = 256;
static int LINE_SIZE = 2048;

extern int errno;


/*
    Appending an array values to an pointer array
*/
void add_pointer(char** returnText, char add[], int* index ,int* SIZE){

    for(int i=0; i<strlen(add); ++i){
        (*returnText)[*index] = add[i];
        (*index)++;
        if(*index > *SIZE){
            *SIZE *= 2;
            *returnText = (char*) w_realloc(*returnText, (*SIZE) * sizeof(char));
        }
    }
}


/*
    It is reading line by line from file 
    and operating over them with the key values one by one
    
    ex: 
    read line convert them for the first rule then apply second convert rule
    the get the next line

*/
char* read_from_file(char* filepath, ReplaceWord rw_t[], int rw_size){
    
    struct flock lock;
    char buffer[BUF_SIZE], line[LINE_SIZE];
    char* bp = NULL, *text_iter, *returnText;

    int fd, count = 0, index = 0, read_bytes = 0,
        total_bytes = 0, enterNewLine = 0;

    fd = w_open(filepath, READ_FLAGS);

    returnText = (char*) w_calloc(FILE_BYTE_SIZE, sizeof(char)); 
    
    text_iter = line;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    fcntl(fd, F_SETLKW, &lock);

    for( ; ; ){
            
        while (((read_bytes = read(fd, buffer, sizeof(buffer))) == -1) 
                && (errno == EINTR)){
            // buffer[read_bytes] = '\n';
            continue;
        }

        if (read_bytes <= 0)    break;
        
        bp = buffer;
        
        while(read_bytes > 0){

            total_bytes++;
            *text_iter = *bp;
            
            if(*text_iter == '\n'){
                
                char res[FILE_BYTE_SIZE];
                for(int i=0; i<FILE_BYTE_SIZE; ++i)
                    res[i] = '\0';
                lseek(fd, total_bytes, SEEK_SET);
                
                for(int i = 0; i<rw_size; ++i){
                    replace(line, rw_t[i], res, FILE_BYTE_SIZE);
                    strcpy(line, res);
                }
                add_pointer(&returnText, res, &index, &FILE_BYTE_SIZE);
                enterNewLine = 1;
                
                break;
            }
            bp++;
            read_bytes--;
            text_iter++;
        }

        if(enterNewLine){
            for(int i = 0; i<LINE_SIZE; ++i)
                line[i] = '\0';
            text_iter = line;
            enterNewLine = 0;
        }else{
            if(strlen(line) < LINE_SIZE)
                break;
            perror("some lines are over flow");
            return returnText;
        }
    }

    // EOF Problem if the file not finish \n
    if(strlen(line) > 0){
        char res[FILE_BYTE_SIZE];
        for(int i = 0; i<LINE_SIZE; ++i){
            line[i] = '\0';
            res[i] = '\0';
        }

        for(int i = 0; i<rw_size; ++i){
            replace(line, rw_t[i], res, FILE_BYTE_SIZE);
            strcpy(line, res);

            // strcpy(line, replaced);
            // free(replaced);
        }
        add_pointer(&returnText, res, &index, &FILE_BYTE_SIZE);
        returnText[strlen(returnText)-2] = '\0';
        returnText[strlen(returnText)-1] = '\0';
    }

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);

    w_close(fd);

    return returnText;
}


/*
    It's writing to a file 
*/
int write_to_file(char* filepath, char* text){

    int fd, index = 0,
        writeByte = 0, 
        total_bytes = 0,
        length = strlen(text);

    struct flock lock;
    char buf[BUF_SIZE];

    char* bp = text;

    fd = w_open_p(filepath, WRITE_FLAGS,0777);

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);

    while(length > 0){
        
        while (((writeByte = write(fd, bp, length)) == -1) 
                && (errno == EINTR)){
            continue;
        }

        if(writeByte < 0)  break;

        length -= writeByte;
        bp += writeByte;
        total_bytes += writeByte;
    }

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    
    w_close(fd);

    return 0;
}

