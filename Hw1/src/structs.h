#ifndef STRUCTS_H_
#define STRUCTS_H

    typedef struct ReplaceWord{
        char* oldWord;
        char* newWord;
        int caseSensetive;
        int isLineStart;
        int isEndOfLine;
    }ReplaceWord;

    ReplaceWord create_replaceWord_2(char* sentence);

#endif