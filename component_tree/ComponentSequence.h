//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_COMPONENTSEQUENCE_H
#define BECCHI_REGEX_COMPONENTSEQUENCE_H

#include <vector>
#include "Component.h"
#include "ComponentClass.h"
#include "ComponentRepeat.h"
#include "ComponentAlternation.h"

class ComponentSequence: public Component {
public:
    std::vector<Component*> children;

    char re_part[1000];

    ~ComponentSequence();

    void add(Component *pComponent);

    int num_concat() override;

    double p_match() override;

    bool decompose(int &threshold, std::__1::bitset<256> &alpha, char* R_pre, char* R_post, int& depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass) override;

    char* get_re_part() override;

    char* get_reverse_re() override ;

    void extract(char* R_pre, char* R_mid, char* R_post, int threshold);
};


#endif //BECCHI_REGEX_COMPONENTSEQUENCE_H
