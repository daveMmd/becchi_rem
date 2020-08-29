//
// Created by 钟金诚 on 2020/8/13.
//

#include "ComponentSequence.h"
#include "../stdinc.h"

void ComponentSequence::add(Component *pComponent) {
    children.push_back(pComponent);
}

int ComponentSequence::num_concat() {
    int num = 0;
    for(std::vector<Component*>::iterator it = children.begin(); it != children.end(); it++){
        num += (*it)->num_concat();
    }
    return num;
}

bool ComponentSequence::decompose(int &threshold, std::bitset<256> &alpha, char* R_pre, char* R_post, int& depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass) {
    bool res = false;
    for(auto & it : children){
        if(threshold <= 0) res = true;
        if(!res && it->decompose(threshold, alpha, R_pre, R_post, depth, first_charClass, last_infinite_charclass)){
            res = true;
            if(typeid(it) == typeid(ComponentClass)) strcat(R_post, it->get_re_part());
        }
        else if(res){
            strcat(R_post, it->get_re_part());
        }
        //record first char class
        /*
        if(alpha.any() && (alpha & (((ComponentClass*) it)->charReach)).any() \
        && typeid(*it) == typeid(ComponentClass) && first_charClass == nullptr) first_charClass = &((ComponentClass*) it)->charReach;
        else if(typeid(*it) == typeid(ComponentRepeat)){
            Component *sub = ((ComponentRepeat*)(it))->sub_comp;
            if(typeid(*sub) == typeid(ComponentClass)){
                if(first_charClass == nullptr) first_charClass = &((ComponentClass*) sub)->charReach;
                if(((ComponentClass*)sub)->charReach.all() && ((ComponentRepeat*)(it))->m_max == _INFINITY) first_charClass = nullptr;
            }
        }*/
    }
    return res;
}

char *ComponentSequence::get_re_part() {
    re_part[0] = '\0';
    for(std::vector<Component*>::iterator it = children.begin(); it != children.end(); it++){
        strcat(re_part, (*it)->get_re_part());
    }
    return re_part;
}

ComponentSequence::~ComponentSequence() {
    for(auto &it: children){
        delete it;
    }
}

void ComponentSequence::extract(char *R_pre, char *R_mid, char *R_post, int threshold) {
    double p_match = 1.0;
    double p_match_minimum = 1.0;
    //used to record cut position
    Component *comp_1;// *comp_2;
    Component *comp_minimum_1 = nullptr, *comp_minimum_2 = nullptr;
    int cut_num = 0; //0(not contain); -1(complete contain); n{cut n repeation}
    int lef_threshold = threshold;

    std::vector<std::bitset<256>* > former_charclass;
    //first calculate the minimum p_match
    for(auto &it: children){
        int tem_cut_num = 0; //(not contain)
        if(p_match == 1.0) comp_1 = it;
        if(typeid(*it) == typeid(ComponentClass)){
            tem_cut_num = -1;
            double tem_p = ((ComponentClass*) it)->p_match();
            p_match *= tem_p;
            lef_threshold -= 1;
            former_charclass.push_back(&((ComponentClass*) it)->charReach);
        }
        //note: may cause explosion
        else if(typeid(*it) == typeid(ComponentRepeat)){
            int m_min = ((ComponentRepeat*) it)->m_min;
            int m_max = ((ComponentRepeat*) it)->m_max;
            if(m_min == 0) goto CUT;

            Component* sub = ((ComponentRepeat *) it)->sub_comp;
            if(typeid(*sub) == typeid(ComponentClass)){
                std::bitset<256> ES1, charclass;

                if(former_charclass.empty()) goto NO_EXPLOSION;
                ES1 = *former_charclass[0];
                charclass = ((ComponentClass *) sub)->charReach;
                if((ES1&charclass).none()){
                    NO_EXPLOSION:
                    int cut = min(lef_threshold, m_min);
                    tem_cut_num = cut;
                    lef_threshold -= cut;
                    p_match *= pow(((ComponentClass *) sub)->p_match(), cut);
                    for(int i = 0; i < cut; i++) former_charclass.push_back(&((ComponentClass *) sub)->charReach);
                }
                else goto CUT;
            }
            else if(typeid(*sub) == typeid(ComponentSequence)){
                //todo
                goto CUT;
            }
            else{// if(typeid(sub) == typeid(ComponentAlternation)){
                goto CUT;
            }

            if(m_min != m_max) goto CUT; //cause length not determined
        }
        else{//Alternation, break
            goto CUT;
        }

        //reach to threshold, CUT
        if(lef_threshold <= 0){
            CUT:
            //p_match_minimum = min(p_match_minimum, p_match);
            if(p_match_minimum > p_match){//update
                p_match_minimum = p_match;
                comp_minimum_1 = comp_1;
                comp_minimum_2 = it;
                cut_num = tem_cut_num;
            }
            lef_threshold = threshold;
            p_match = 1.0;
            former_charclass.clear();
        }
    }

    //no R_post
    if(p_match < p_match_minimum){
        comp_minimum_1 = comp_1;
        comp_minimum_2 = *children.end();
        cut_num = -1;//complete contain
    }

    //second extract R_pre, R_mid and R_post
    char* cur_s = R_pre;
    for(auto &it: children){
        if(it == comp_minimum_1){// begin() -> comp_minimum_1 () --> R_pre
            cur_s = R_mid;
        }
        else if(it == comp_minimum_2){//according to cut_num, decide how to cut comp_minimum_2
            if(cut_num == -1) strcat(cur_s, it->get_re_part());
            else if(cut_num == 0) strcat(R_post, it->get_re_part());
            else{ //cut from middle
                Component* sub = ((ComponentRepeat *)it)->sub_comp;
                int tem_min = ((ComponentRepeat *)it)->m_min;
                int tem_max = ((ComponentRepeat *)it)->m_max;
                ((ComponentRepeat *)it)->m_min = cut_num;
                ((ComponentRepeat *)it)->m_max = cut_num;
                strcat(cur_s, it->get_re_part());

                ((ComponentRepeat *)it)->m_min = tem_min - cut_num;
                if(tem_max != _INFINITY) ((ComponentRepeat *)it)->m_max = tem_max - cut_num;
                else ((ComponentRepeat *)it)->m_max = tem_max;
                strcat(R_post, it->get_re_part());
            }
            cur_s = R_post;
            continue;
        }
        strcat(cur_s, it->get_re_part());
    }
}

double ComponentSequence::p_match() {
    double p = 1.0;
    for(auto &it: children) p *= it->p_match();
    return p;
}

char *ComponentSequence::get_reverse_re() {
    re_part[0] = '\0';
    for(auto it = children.rbegin(); it != children.rend(); it++){
        strcat(re_part, (*it)->get_re_part());
    }
    return re_part;
}
