//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_COMPONENT_H
#define BECCHI_REGEX_COMPONENT_H
#define DEAFAULT_THRESHOLD 100

#include <bitset>

class Component {
public:
    virtual int num_concat() = 0;

    virtual double p_match() = 0;

    /*
     * threshold: desired max #states
     * alpha: former char class with infinite loop
     * R_pre: target re prefix
     * R_post: target re postfix
     * int depth: current depth
     * first_charClass: first char class behind alpha
     * last_infinite_charclass: latest char class with infinite loop(only affect one component), used to help determine first_charClass
     * */
    virtual bool decompose(int &threshold, std::__1::bitset<256> &alpha, char* R_pre, char* R_post, int &depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass) = 0;

    virtual char* get_re_part() = 0;
    virtual char* get_reverse_re() = 0;
};



#endif //BECCHI_REGEX_COMPONENT_H
