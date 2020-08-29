//
// Created by 钟金诚 on 2020/8/13.
//

#include "util.h"
#include "../stdinc.h"

std::list<char *>* read_regexset(char* fname){
    FILE* file = fopen(fname, "r");
    if(file == NULL) fatal("read_regexset: file NULL.");
    auto regex_list = new std::list<char *>();
    char tmp[1000];
    while(1){
        if(fgets(tmp, 1000, file)==NULL) break;
        if(tmp[0]=='#') continue;
        int len=strlen(tmp);
        if(len == 1 && tmp[len-1] == '\n') continue; //jump empty line
        if(tmp[len-1] == '\n')
        {
            tmp[len-1]='\0';
        }
        char *re= (char*)malloc(1000);
        strcpy(re, tmp);
        regex_list->push_back(re);
    }
    fclose(file);
    return regex_list;
}