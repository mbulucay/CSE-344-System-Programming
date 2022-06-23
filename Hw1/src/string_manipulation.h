
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"

#ifndef STRING_MANIPULATION_H_
#define STRING_MANIPULATION_H_


    int get_key_value_pairs(ReplaceWord key_values[], int size, char* str);
    int reg_key_value(ReplaceWord old_key_values[],  ReplaceWord new_key_values[], int old_size,int new_size );

    void replace(char* sentence, ReplaceWord replaceWord, char res[], int size);
    char* new_replace(char* text, ReplaceWord replaceWord);

    int my_strlen(char*);
    char** parse_string(char* str, char* parser);
    char* str_replace(const char* text, ReplaceWord replaceWord);

    char** parse_slash(char* str);

#endif