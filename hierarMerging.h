//
// Created by 钟金诚 on 2020/5/16.
//

#ifndef BECCHI_REGEX_HIERARMERGING_H
#define BECCHI_REGEX_HIERARMERGING_H

#include "stdinc.h"
#include "dfa.h"

DFA *dfas_merge(list<DFA*> *tomerge_lis, int k=2);

DFA *dfas_merge_improve(list<DFA*> *tomerge_lis, int k=2);

DFA *hm_nfa2dfa(list<NFA*> *nfa_list, int k=2,bool improved=true);

DFA *hm_dfalist2dfa(list<DFA*> *ptr_dfalist, int k=2, bool improved=true);

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

#endif //BECCHI_REGEX_HIERARMERGING_H
