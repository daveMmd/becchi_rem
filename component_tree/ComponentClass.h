//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_COMPONENTCLASS_H
#define BECCHI_REGEX_COMPONENTCLASS_H

#include <bitset>
#include "Component.h"

class ComponentClass: public Component {
public:
    std::bitset<256> charReach;
    char re_part[1000]; //used to concat re

    ComponentClass(){
        charReach.reset();
    }

    void insert(int);
    void insert(int, int);

    void negate();

    int head();

    int num_concat() override;

    char* get_re_part() override;

    char* get_reverse_re();

    bool decompose(int &threshold, std::__1::bitset<256> &alpha, char* R_pre, char* R_post, int &depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass) override;

    double p_match() override{
        return charReach.count() * 1.0 / charReach.size();
    }
};


#endif //BECCHI_REGEX_COMPONENTCLASS_H
