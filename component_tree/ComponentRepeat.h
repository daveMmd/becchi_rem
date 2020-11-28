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
    //used to save unchanged value (for algorithm use)
    int _m_min;
    int _m_max;

    ~ComponentRepeat() override;
    int num_concat() override;
    bool decompose(double cur_pmatch, int &threshold, std::__1::bitset<256> &alpha, char* R_pre, char* R_post, int &depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass, bool top) override;

    char* get_re_part() override;

    char* get_reverse_re() override ;

    double p_match() override;

    bool is_dotstar();

    void save_value(){
        _m_min = m_min;
        _m_max = m_max;
    };

    void recover_value(){
        m_min = _m_min;
        m_max = _m_max;
    }
};


#endif //BECCHI_REGEX_COMPONENTREPEAT_H
