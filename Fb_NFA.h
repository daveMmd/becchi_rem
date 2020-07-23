//
// Created by 钟金诚 on 2020/7/22.
//

#ifndef BECCHI_REGEX_FB_NFA_H
#define BECCHI_REGEX_FB_NFA_H


#include "Fb_DFA.h"

//#include "dfa.h"

class Fb_NFA {
public:
    int _size;
    set<state_t> ***stt;
    set<state_t> start_states;
    set<state_t> accept_states;

    Fb_NFA(int size);
    ~Fb_NFA();
    Fb_DFA* nfa2dfa();

    void to_dot(char* fname, char* title);
};


#endif //BECCHI_REGEX_FB_NFA_H
