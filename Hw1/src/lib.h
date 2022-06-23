#include "string_manipulation.h"

#ifndef LIB_H_
#define LIB_H_

    char* read_from_file(char* filepath, ReplaceWord rw_t[], int rw_size);
    
    void print_temporary_file(char* sentence);

    int write_to_file(char* filepath, char* text);

#endif