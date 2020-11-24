//
// Created by 钟金诚 on 2020/11/20.
//

#include <unordered_map>
#include <list>
#include "Fb_DFA.h"
#include "dfa.h"
#include "prefix_DFA.h"
#include "trace.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp> //序列化STL容器要导入
#include <fstream>
#include <iostream>

using namespace std;
#define MAX_CHARS_PER_LINE 5000

char *trace_fname = nullptr;
unordered_map<uint32_t, list<uint16_t>*> *mapping_table;
vector<prefix_DFA*> vec_prefixdfa;
int DEBUG = 0;
int VERBOSE = 0;

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

void load_prefixDfas(){
    ifstream ifs("vec_prefix_dfas.bin");
    boost::archive::text_iarchive ia(ifs);
    ia >> vec_prefixdfa;
    ifs.close();

    //load dfas in prefix dfas
    for(auto &it: vec_prefixdfa){
        it->complete_re = const_cast<char *>(it->complete_re_string.c_str());
        it->re = const_cast<char *> (it->re_string.c_str());

        it->import_dfa();//not need to import in FPGA-CPU arch, can be used for matching result verification
        prefix_DFA* parentNode = it->parent_node;
        while(parentNode != nullptr) {
            parentNode->import_dfa();
            parentNode = parentNode->parent_node;
        }
        prefix_DFA* childNode = it->next_node;
        while(childNode != nullptr){
            childNode->import_dfa();
            childNode = childNode->next_node;
        }
    }
}

int prefix_DFA::gid = 0;

void test_using_software(){
    list<prefix_DFA*> lis_prefixdfa;
    lis_prefixdfa.insert(lis_prefixdfa.begin(), vec_prefixdfa.begin(), vec_prefixdfa.end());

    if (trace_fname){
        printf("simulating trace traverse, trace file:%s\n", trace_fname);
        auto tr=trace(trace_fname);
        tr.traverse(&lis_prefixdfa);
        //delete tr;
    }
}

int main(int argc, char **argv){
    if(argc >= 2) trace_fname = argv[1];

    load_mapping();
    load_prefixDfas();

    //test the correctness
    test_using_software();

    return 0;
}