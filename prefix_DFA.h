//
// Created by 钟金诚 on 2020/8/20.
//

#ifndef BECCHI_REGEX_PREFIX_DFA_H
#define BECCHI_REGEX_PREFIX_DFA_H
#include "Fb_DFA.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

/*
 * 用于模拟fpga multi dfa 架构的CPU负担
 * */
class prefix_DFA{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //ar& prefix_dfa;
        ar& id;

        ar& next_node;
        ar& parent_node;
        ar& depth;
        ar& flag_activate_once;
        ar& is_activate;
        ar& is_silent;

        ar& complete_re_string;
        ar& re_string;
    }
public:
    static int gid; //prefixDFA global id

    char *complete_re; //for debug : correspond to original rule
    //string complete_re;
    char *re; //debug: correspond to the re part compiled in dfa
    //string re;
    Fb_DFA* fbDfa;
    DFA* prefix_dfa;
    prefix_DFA* next_node;
    prefix_DFA* parent_node;

    int id;//prefixDFA id

    int depth;//determined depth of mid part (only used in mid-DFA)

    //activate next post dfa(i.e., next_node)
    bool flag_activate_once; //post start with .* like, only need to be activated once
    bool is_activate;//cowork with flag_activate_once
    bool is_silent;

    string complete_re_string;
    string re_string;

    prefix_DFA(){
        complete_re = nullptr;
        depth = 0;
        id = (gid++);
        is_activate = false;
        flag_activate_once = false;
        is_silent = false;
        re = nullptr;
        fbDfa = nullptr;
        prefix_dfa = nullptr;
        next_node = nullptr;
        parent_node = nullptr;
    }

    ~prefix_DFA(){
        delete re;
        delete fbDfa;
        delete prefix_dfa;
        delete next_node;
        //delete parent_node; //dup delete
    }

    //init all working varibles
    void init(){
        prefix_DFA* node = this;
        while(node->next_node != nullptr){
            node->is_activate = false;
            node->is_silent = false;
            node = node->next_node;
        }
        node->is_activate = false;
        node->is_silent = false;
    }

    void set_activate_once() //set activate_once for cur prefix_DFA
    {
        prefix_DFA* node = this;
        while(node->next_node != nullptr){
            node = node->next_node;
        }
        node->flag_activate_once = true;
    };

    void add_dfa(DFA* dfa){
        if(prefix_dfa == nullptr) prefix_dfa = dfa;
        else{
            if(next_node == nullptr) {
                next_node = new prefix_DFA();
                next_node->parent_node = this;
            }
            next_node->add_dfa(dfa);
        }
    }

    void add_fbdfa(Fb_DFA* fb_dfa){
        fbDfa = fb_dfa;
    }

    //call to not perform dfa transitions any more
    void self_silent(){
        is_silent = true;
        if(parent_node != nullptr) parent_node->self_silent();
    }

    //when match call to get post dfa
    prefix_DFA* get_post_dfa(){
        if(flag_activate_once) self_silent(); //仅触发一次，因此当前dfa以及当前dfa的父dfa均不必再匹配
        if(flag_activate_once && is_activate) return nullptr;
        is_activate = true;

        if(next_node == nullptr) return nullptr;
        if(next_node->prefix_dfa == nullptr) return nullptr;
        return next_node;
    }

    void add_re(char* s){
        if(re == nullptr) {
            re = (char*) malloc(1000);
            strcpy(re, s);
        }
        else{
            if(next_node == nullptr) {
                next_node = new prefix_DFA();
                next_node->parent_node = this;
            }
            next_node->add_re(s);
        }
    }

    /*print how re is decomposed*/
    void debug(FILE* file, char *regex){
        if(parent_node == nullptr && next_node == nullptr) return;
        fprintf(file, "\n#####\n%s\n#####\n", regex);
        if(parent_node != nullptr){
            fprintf(file, "***extract pre***:\n");
            prefix_DFA* node = parent_node;
            while(node != nullptr){
                fprintf(file, "%s\n", node->re);
                node = node->next_node;
            }
        }

        fprintf(file, "***in bram***:\n");
        fprintf(file, "%s\n", re);

        if(next_node != nullptr){
            fprintf(file, "***extract post***:\n");
            prefix_DFA* node = next_node;
            while(node != nullptr){
                fprintf(file, "%s\n", node->re);
                node = node->next_node;
            }
        }
    }

    void dump_dfa(){
        char filename[100];
        sprintf(filename, "./dfas/dfa_%d.bin", id);
        FILE* f = fopen(filename, "w");
        if(f != nullptr){
            prefix_dfa->put(f);
            fclose(f);
        }
        else fprintf(stderr, "cannot open the file!\n");
    }

    void import_dfa(){
        char filename[100];
        sprintf(filename, "./dfas/dfa_%d.bin", id);
        FILE* f = fopen(filename, "r");
        if(f != nullptr){
            //if(prefix_dfa == nullptr)
            prefix_dfa = new DFA();
            prefix_dfa->get(f);
            fclose(f);
        }
        else fprintf(stderr, "cannot open the file!\n");
    }
};
#endif //BECCHI_REGEX_PREFIX_DFA_H
