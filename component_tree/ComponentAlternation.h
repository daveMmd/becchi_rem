//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_COMPONENTALTERNATION_H
#define BECCHI_REGEX_COMPONENTALTERNATION_H

#include <vector>
#include "Component.h"
#include "ComponentSequence.h"

class ComponentAlternation: public Component {
public:
    char re_part[1000];

    std::vector<Component*> children;

    ~ComponentAlternation();

    void add(Component *pComp);

    int num_concat() override;

    double p_match() override;

    char* get_re_part() override;

    char* get_reverse_re() override;

    bool decompose(int &threshold, std::__1::bitset<256> &alpha, char* R_pre, char* R_post, int& depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass, bool top = false) override;
};


#endif //BECCHI_REGEX_COMPONENTALTERNATION_H
