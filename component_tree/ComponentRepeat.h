//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_COMPONENTREPEAT_H
#define BECCHI_REGEX_COMPONENTREPEAT_H

#include "Component.h"
#include "ComponentClass.h"
#include "ComponentSequence.h"
class ComponentRepeat: public Component {
public:
    ComponentRepeat(Component *sub, int lb, int ub);
    char re_part[1000];

    Component* sub_comp;
    int m_min;
    int m_max;

    ~ComponentRepeat();
    int num_concat() override;
    bool decompose(int &threshold, std::__1::bitset<256> &alpha, char* R_pre, char* R_post, int &depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass) override;

    char* get_re_part() override;

    char* get_reverse_re() override ;

    double p_match() override;
};


#endif //BECCHI_REGEX_COMPONENTREPEAT_H
