#include "string_manipulation.h"
#include "syscall_wrappers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


ReplaceWord create_replaceWord_2(char* sentence){

    ReplaceWord rw;
    int i = 0, j = 0;

    char** parse = parse_slash(sentence);

    rw.caseSensetive = (parse[2][0] == 'i') ? 1 : 0;
    rw.isLineStart = (parse[0][0] == '^') ? 1 : 0;
    rw.isEndOfLine = (parse[0][strlen(parse[0]) - 1] == '$') ? 1 : 0;

    rw.oldWord = (char*) w_calloc(strlen(parse[0]), sizeof(char));
    while(parse[0][i] != '\0'){
        if(parse[0][i] != '^' && parse[0][i] != '$'){
            rw.oldWord[j] = parse[0][i]; 
            j++;
        }
        i++;
    }
    i = 0, j = 0;
    rw.newWord = (char*) w_calloc(strlen(parse[1]), sizeof(char));
    while(parse[1][i] != '\0'){
        rw.newWord[i] = parse[1][i]; 
        i++;
    }

    free(parse[0]);
    free(parse[1]);
    free(parse[2]);
    free(parse);

    return rw;
}
