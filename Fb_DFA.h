//
// Created by 钟金诚 on 2020/7/22.
//

#ifndef BECCHI_REGEX_FB_DFA_H
#define BECCHI_REGEX_FB_DFA_H
#define MAX_BIG_SATES_NUMBER 100
#define MAX_STATES_NUMBER 212

#include "dfa.h"
#include <unordered_map>
#define INVALID_ID 65535
//#include "Fb_NFA.h"

class Fb_DFA {
private:
    int cons2state_num;
    int smallsate_num;
    int entry_allocated;
public:
    /* state involving no progression */
    state_t dead_state;

    state_t **state_table;
    int _size;
    int *is_accept;

    union{
        uint16_t ind_prefixdfa_single; //临时记录单条规则编译时对应的prefixDFA
        uint16_t ind_re_report; //或是记录匹配哪一条正则表达式
    };


    list<uint16_t >** accept_list; //记录匹配时，对应激活哪一个prefix_DFA(单个状态可能对应多个prefixDFA)

    explicit Fb_DFA(uint16_t ind = INVALID_ID);
    explicit Fb_DFA(DFA* dfa, uint16_t ind = INVALID_ID);
    ~Fb_DFA();

    void to_dot(char* fname, char* title);

    /*after fbdfa is generated, init accept list*/
    void init_accept_rules();

    /*Brzozowski’s algorithm*/
    Fb_DFA* minimise();

    /*Minimization using Equivalence Theorem*/
    /*one bug: 仅适用于单条规则时，所有accept state 等价； 若多条规则的fb_dfa, 不同接受规则的accept state区分*/
    /*one bug: 由于视所有相同具有转移边的状态为相同状态，仅能区分状态是否为accept，不能用于判定哪一条规则成功匹配*/
    Fb_DFA* minimise2();

    void add_transition(state_t s, int c, state_t d);
    state_t add_state();

    /* returns the next_state from state on symbol c */
    state_t get_next_state(state_t state, int c);

    int size();
    int less2states();
    int cons2states();
    int get_smallstate_num();
    int get_large_states_num();

    Fb_DFA* converge(Fb_DFA *);
    bool small_enough();

    float get_ratio();

    void* to_BRAM(uint16_t dfaid, map<uint32_t, list<uint16_t>*> *mapping_table);/*gen STT bit stream compatible for FPGA*/

    bool is_bigstate(state_t state);
};


#endif //BECCHI_REGEX_FB_DFA_H
