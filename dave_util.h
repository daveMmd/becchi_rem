//
// Created by 钟金诚 on 2019/8/27.
//

#ifndef BECCHI_REGEX_DAVE_UTIL_H
#define BECCHI_REGEX_DAVE_UTIL_H

#include "stdinc.h"
#include "dfa.h"
/*
#include "nfa.h"
#include "parser.h"
using namespace std;*/
//#include <sstream>

#define FOREACH_DFA_LIST(list_id,it) \
	for(list<DFA *>::iterator it=list_id->begin();it!=list_id->end();++it)
#define KMAX 10
//#define DEBUG_DAVE


class DfaStateLis
{
public:
    state_t value[KMAX];//k max 10
    int size;
    DfaStateLis(int s):size(s){};

    void set_value(int pos, state_t v){
        value[pos] = v;
    }
    bool operator<(const DfaStateLis &d) const //注意这里的两个const
    {
        //return (age < p.age) || (age == p.age && name.length() < p.name.length()) ;
        for(int i=0; i < size; i++){
            if(value[i] < d.value[i]) return true;
            if(value[i] > d.value[i]) return false;
        }
        return false;
    }

    state_t operator[](int pos){
        return value[pos];
    }
};

inline void debugPrint(const char* s){
#ifdef DEBUG_DAVE
    fprintf(stderr, s);
#endif
}

inline void debugPrint(int a){
#ifdef DEBUG_DAVE
    fprintf(stderr, "%d", a);
#endif
}

inline void dumpDFA(char * fname, DFA *dfa){
#ifdef DEBUG_DAVE
    /*FILE * file = fopen(fname, "w");
    dfa->dump(file);
    fclose(file);*/
    // DOT file generation
    FILE *dot_file=fopen(fname,"w");
    fprintf(stderr,"\nExporting to DOT file %s ...\n",fname);
    char string[100] = "any title";
    dfa->to_dot(dot_file, string);
    fclose(dot_file);
#endif
}
#endif //BECCHI_REGEX_DAVE_UTIL_H
