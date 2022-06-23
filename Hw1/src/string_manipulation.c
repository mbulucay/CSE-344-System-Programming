#include "string_manipulation.h"
#include "syscall_wrappers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
    It is counting the lenght of getting rule sentence 
    not counting special objects
*/
int my_strlen(char* str){

    if(str == NULL) return 0;

    int length = 0;
    for(int i=0; i<strlen(str); ++i){
    
        if(str[i] == '['){
            while (str[i++] != ']');
            length++;
        }
        if(str[i] >= 'a' && str[i] <= 'z' || str[i] >= 'A' && str[i] <= 'Z' || str[i] >= '0' && str[i] <= '9')
            length++;
    }

    return length;
}


char makeLower(char c){
    return (c>='A' && c<='Z') ? c+32 : c;
}


int eq_case(const char ch1, const char ch2, int caseSensetive){
    return caseSensetive ? ch1 == ch2 : makeLower(ch1) == makeLower(ch2);
}


void replace(char* text, ReplaceWord replaceWord, char res[], int size){

    int length = strlen(text);
    int old_w_len = my_strlen(replaceWord.oldWord);
    int new_w_len = strlen(replaceWord.newWord);

    int rfd = 0;
    int tfd = 0;
    int in_tfd;
    int in_cursor, is_in, did = 1;

    // char* res = (char*) w_calloc(length, sizeof(char));

    char ma, last_in = '\0';
    while(text[tfd] != '\0'){

        int old_w_cursor = 0;   // old word cursor
        int match = 0;          //matching character number
        int keep = 1;           //keep look for a key letter iterator
        int repeat = 0;

        in_tfd = tfd;           //text file in while icinde tmp iteratoru
        while (match < old_w_len && keep){ 

            switch (replaceWord.oldWord[old_w_cursor])
            {
                case '^':
                {
                    // old_w_cursor++;
                    in_tfd--;
                }
                    break;

                case '[':
                {
                        /*
                            When the [ digit has come we are going forwad until across with ] and check the 
                            searching key is in these two bracket if it is in the necessary cursor setting are set
                            if it is not we are going out of inner while loop
                        */
                        in_cursor = old_w_cursor;   //old value icin ne kadar deger artirman gerektigi
                        is_in = 0;                  //bool value

                        while(replaceWord.oldWord[in_cursor] != ']'){
                            if(eq_case(text[in_tfd],replaceWord.oldWord[in_cursor],replaceWord.caseSensetive)){
                                is_in = 1;
                                last_in = text[in_tfd];
                            }
                            in_cursor++;                      
                        }

                        if(is_in == 1){
                            old_w_cursor += in_cursor;
                            if(replaceWord.oldWord[0] == '['){
                                old_w_cursor++;
                            }  
                            match++;

                        }else{
                            keep = 0;
                        }
                }
                    break;

                case '*':
                {
                    /*
                        When the * character is occur we are forwarding the cursor until accross with different character
                        and count the repeated times for the cursor setting
                    */
                    char repeatCharacter;
                    
                    if(replaceWord.oldWord[old_w_cursor-1] == ']')
                        repeatCharacter = last_in;
                    else
                        repeatCharacter = replaceWord.oldWord[old_w_cursor-1];
              
                    if(replaceWord.oldWord[old_w_cursor+1] != '\0' && replaceWord.oldWord[old_w_cursor+1] == '*'){
                        match++;
                        old_w_cursor++;
                    }
                    while(eq_case(repeatCharacter,text[in_tfd],replaceWord.caseSensetive)){
                        in_tfd++;
                        repeat++;
                    }
                    old_w_cursor++;
                    in_tfd--;
                }
                    break;

                default:
                {
                    /*
                        This is just normal comparison according to the case sensetive or not
                    */
                    if(eq_case(replaceWord.oldWord[old_w_cursor], text[in_tfd],replaceWord.caseSensetive)){
                        old_w_cursor++;
                        match++;
                    }else{
                        keep = 0;
                    }
                }
            }
            in_tfd++;
        }

        if(match != old_w_len && replaceWord.isLineStart == 1){
            for(int i = 0; i<size; ++i){
                res[i] = text[i];
            }
            return;
        }
        if(match == old_w_len && replaceWord.isLineStart){
            for(int i=0; i<new_w_len; ++i)
                res[i] = replaceWord.newWord[i];
            
            for(int i = 0; i<length; ++i)
                res[new_w_len+i] = text[in_tfd+i];
            
            return;
        }

        /*
            If matching approved we are putting the new sentence for result array else we are putting the next character
        */
        if(match == old_w_len && (((replaceWord.isEndOfLine && (text[tfd+match+repeat] == '\n')) || (replaceWord.isEndOfLine == 0))) ){
            for(int i=0; i<new_w_len; ++i)
                res[rfd+i] = replaceWord.newWord[i];
            tfd += match+repeat;
            rfd += new_w_len;
        }else{
            res[rfd] = text[tfd];
            rfd++;
            tfd++;
        }
    }

    res[rfd] = '\0';
    return;
    // return res;
}


int get_key_value_pairs(ReplaceWord key_values[], int size, char* str){
    
    int count = 0;
    char** parsed = parse_string(str, ";");

    while(parsed[count] != NULL){
        key_values[count] = create_replaceWord_2(parsed[count]);
        free(parsed[count]);
        count++;
    }
    free(parsed);

    return count;
}


int number_of_characters(char* str, char c){
    int count = 0, index = 0;
    while(str[index] != '\0'){
        if(str[index] == c)
            count++;
        index++;
    }
    return count;
} 


char** parse_string(char* str, char* parser){

    int index = 0;

    int count = number_of_characters(str, *parser);
    if(*parser == ';')
        count+=2;

    char** words = (char**) w_calloc(sizeof(char*),count);

    for(int i=0; i<count; i++)
        words[i] = (char*) w_calloc(strlen(str), sizeof(char));
    
    int i = 0;
    int c = 0;
    
    while (str[i] != '\0'){
        if(str[i] == *parser){
            words[index][c] = '\0';
            index++;
            c = 0;
        }else{
            words[index][c] = str[i];
            c++;
        }
        i++;
    }

    free(words[count-1]);
    words[count-1] = NULL;

    return words;
}


char** parse_slash(char* str){
    
    int count = number_of_characters(str, '/');

    char** words = (char**) w_calloc(sizeof(char*),count+1);

    for(int i=0; i<count+1; i++)
        words[i] = (char*) w_calloc(strlen(str), sizeof(char));

    int i = 1;
    int c = 0;
    int index = 0;

    while(str[i] != '\0'){
        if(str[i] == '/'){
            words[index][c] = '\0';
            index++;
            c = 0;
       }else{
            words[index][c] = str[i];
            c++;
        }
        i++;
    }
    free(words[count]);
    words[count] = NULL;

    return words;
}

