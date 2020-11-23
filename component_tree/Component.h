//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_COMPONENT_H
#define BECCHI_REGEX_COMPONENT_H
#define DEAFAULT_THRESHOLD 100
#define PMATCH_THRESHOLD 0.000000000001 //pmatch for five chars 约等于 10^-12
//#define PMATCH_THRESHOLD 0 //等价于 不使用pmatch
#define PMATCH_ANCHOR 0.001
//#define PMATCH_THRESHOLD 0

#include <bitset>
#include <list>
//#include "decompose.h"
extern std::list<char*> lis_R_pre;
extern std::list<char*> lis_R_post;

class Component {
public:
    virtual ~Component() = default;

    virtual int num_concat() = 0;

    virtual double p_match() = 0;

    /*
     * double cur_pmatch: cur p_matchh, (> 1 not to update it thus not use it)
     * threshold: desired max #states
     * alpha: former char class with infinite loop
     * R_pre: target re prefix
     * R_post: target re postfix
     * int depth: current depth
     * first_charClass: first char class behind alpha
     * last_infinite_charclass: latest char class with infinite loop(only affect one component), used to help determine first_charClass
     * bool top: assist to decompose alternation(|)
     * */
    virtual bool decompose(double cur_pmatch, int &threshold, std::__1::bitset<256> &alpha, char* R_pre, char* R_post, int &depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass, bool top = false) = 0;

    virtual char* get_re_part() = 0;
    virtual char* get_reverse_re() = 0;
};



#endif //BECCHI_REGEX_COMPONENT_H
