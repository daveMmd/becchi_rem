//
// Created by 钟金诚 on 2019/8/27.
//
/*
#include "dave_util.h"

inline void debugPrint(const char* s){
#ifdef DEBUG_DAVE
    fprintf(stderr, s);
#endif
}

inline void debugPrintLis(DfaStateLis lis){
#ifdef DEBUG_DAVE
    string s="";
    stringstream ss;
    ss<<"\n";
    for(int i=0; i<lis.size; i++) ss << lis[i] << " ";//s = s + lis[i] + " ";
    ss<<"\n";
    debugPrint(ss.str().c_str());

#endif
}
inline void debugPrintNfaSize(NFA *nfa){
#ifdef DEBUG_DAVE
    debugPrint(("\nnfa size"+to_string(nfa->size())+"\n").c_str());
#endif
}


void output_nfa(NFA* nfa, char * prefix,int i){
    if(nfa!=NULL){
        char string[100];
        char out_file[100];
        sprintf(out_file,"./dump/%s_nfa%d", prefix, i);
        FILE *nfa_dot_file=fopen(out_file,"w");
        if(DEBUG) fprintf(stderr,"\nExporting to NFA DOT file %s ...\n", out_file);
        sprintf(string,"source: %s",prefix);
        nfa->to_dot(nfa_dot_file, string);
        fclose(nfa_dot_file);
    }
}
*/