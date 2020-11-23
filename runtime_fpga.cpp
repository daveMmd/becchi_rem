//
// Created by 钟金诚 on 2020/11/20.
//

#include <unordered_map>
#include <list>
#include "Fb_DFA.h"

using namespace std;
#define MAX_CHARS_PER_LINE 5000

unordered_map<uint32_t, list<uint16_t>*> *mapping_table;

void load_mapping(){
    FILE *f = fopen("./mapping.txt", "r");

    mapping_table = new unordered_map<uint32_t, list<uint16_t>*>();
    char buff[MAX_CHARS_PER_LINE];
    int line_cnt = 0;
    while(fgets(buff, MAX_CHARS_PER_LINE, f) != nullptr){
        //printf("line %d\n", line_cnt++);
        char delims[] = " ";
        char *res = strtok(buff, delims);
        int ele_cnt = 0;
        uint32_t key = -1;
        auto *lis = new list<uint16_t>();
        while(res != nullptr){
            uint16_t accept_prefix;
            if(ele_cnt){
                sscanf(buff, "%u", &accept_prefix);
                lis->push_back(accept_prefix);
            }
            else{
                sscanf(buff, "%u", &key);
            }
            //printf("ele:%d res:%s\n", ele_cnt++, res);
            //read next element
            res = strtok(nullptr, delims);
        }
        (*mapping_table)[key] = lis;
    }

    fclose(f);
}


int main(){

    load_mapping();



    return 0;
}