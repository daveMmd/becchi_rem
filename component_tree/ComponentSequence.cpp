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

bool ComponentSequence::decompose(double cur_pmatch, int &threshold, std::bitset<256> &alpha, char* R_pre, char* R_post, int& depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass, bool top) {
    if(cur_pmatch < PMATCH_THRESHOLD) return true;

    /*backup*/
    double _cur_pmatch = cur_pmatch;
    int _threshold = threshold;
    char _R_pre[1000], _R_post[1000];
    int _depth = depth;
    std::bitset<256> _alpha, _first_charClass, _last_infinite_charclass;
    strcpy(_R_pre, R_pre);
    strcpy(_R_post, R_post);
    _alpha = alpha;
    _first_charClass = *first_charClass;
    _last_infinite_charclass = *last_infinite_charclass;

    int last_dotstar_pos = -1; //to locate the position of the last .*
    int pos = 0;
    for(auto &it: children){
        pos++;
        if(typeid(*it) == typeid(ComponentRepeat))
        {
            if(((ComponentRepeat*)it)->is_dotstar()) last_dotstar_pos = pos;
            ((ComponentRepeat*)it)->save_value();
        }

    }

    bool res = false;
    pos = 0;

    /*if anchor rule, start_pmatch *= 0.001 (PMATCH_ANCHOR)*/
    double start_pmatch = 1;
    if(top && last_infinite_charclass->none()) start_pmatch *= PMATCH_ANCHOR;

    for(auto & it : children){
        //printf("pos:%d, cur_match:%llf\n", pos, cur_pmatch);
        pos++;
        //roll-back to try to contain last .*
        if(top && pos <= last_dotstar_pos) cur_pmatch = 2;
        else if(_cur_pmatch <= 1 && cur_pmatch >= 1) cur_pmatch = start_pmatch;

        if(threshold <= 0) res = true;
        if(!res && it->decompose(cur_pmatch, threshold, alpha, R_pre, R_post, depth, first_charClass, last_infinite_charclass, top)){
            res = true;
            if(typeid(*it) == typeid(ComponentClass)) strcat(R_post, it->get_re_part());
        }
        else if(res){
            strcat(R_post, it->get_re_part());
            pos--;
        }
        //update cur_pmatch. for cur_pmatch > 1, not to update
        if(cur_pmatch <= 1) cur_pmatch = cur_pmatch * it->p_match();
    }

    /*可提取最后一个dotstar*/
    if(_cur_pmatch <= 1 && top && last_dotstar_pos >= 0 && pos > (last_dotstar_pos + 1)) g_if_contain_dotstar = true;
    /*未成功提取最后一个.*-like part*/
    if(_cur_pmatch <= 1 && top && pos <= (last_dotstar_pos + 1) && res){
        //roll back two global members
        lis_R_pre.clear();
        lis_R_post.clear();
        //roll back all the changed local members
        //pos = 0;
        cur_pmatch = _cur_pmatch;
        res = false;
        threshold = _threshold;
        alpha = _alpha;
        strcpy(R_pre, _R_pre);
        strcpy(R_post, _R_post);
        depth = _depth;
        *first_charClass = _first_charClass;
        *last_infinite_charclass = _last_infinite_charclass;

        //得到可以覆盖的最后一个.*的位置
        last_dotstar_pos = -1;
        int new_pos = 0;
        for(auto &it: children){
            if(typeid(*it) == typeid(ComponentRepeat)){
                ((ComponentRepeat*)it)->recover_value();
            }
            new_pos++;
            if(new_pos >= (pos-1)) continue;
            if(typeid(*it) == typeid(ComponentRepeat))
                if(((ComponentRepeat*)it)->is_dotstar()) {
                    last_dotstar_pos = new_pos;
                    g_if_contain_dotstar = true;/*可提取至少一个dotstar*/
                }
        }

        pos = 0;//roll back pos

        /*重复前一次动作，除了last_dotstar_pos改变外，其他均不变*/
        for(auto & it : children){
            pos++;
            if(top && pos <= last_dotstar_pos) cur_pmatch = 2;
            else if(_cur_pmatch <= 1 && cur_pmatch >= 1) cur_pmatch = start_pmatch;

            if(threshold <= 0) res = true;
            if(!res && it->decompose(cur_pmatch, threshold, alpha, R_pre, R_post, depth, first_charClass, last_infinite_charclass, top)){
                res = true;
                if(typeid(*it) == typeid(ComponentClass)) strcat(R_post, it->get_re_part());
            }
            else if(res){
                pos--;
                strcat(R_post, it->get_re_part());
            }
            if(cur_pmatch <= 1) cur_pmatch = cur_pmatch * it->p_match();
        }
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

            if(m_min != m_max) goto CUT; //because length not determined
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

void ComponentSequence::extract_simplest(char *R_pre, char *R_mid, char *R_post) {

    uint32_t char_num_minimum = 0xffffffff;
    int simplest_position = -1;
    int last_position = 0;
    //used to record cut position

    //first calculate the position where simplest cut occurs
    for(int i = 0; i < children.size(); i++) {
        uint32_t char_num = 0;
        double p_match = 1.0;
        if(i==0 && flag_anchor) p_match *= PMATCH_ANCHOR;
        for(int j = i; j < children.size(); j++){
            Component* comp_j = children[j];
            if(typeid(*comp_j) == typeid(ComponentClass)){
                p_match *= comp_j->p_match();
                char_num += ((ComponentClass*)comp_j)->charReach.size();
            }
            else{//其他均暂不处理
                break;
            }
            /*判定re是否满足最高匹配概率要求*/
            if(p_match <= PMATCH_THRESHOLD && char_num < char_num_minimum){
                char_num_minimum = char_num;
                simplest_position = i;
                last_position = j;
                break;
            }
        }
    }

    if(simplest_position == -1) return; /*FAIL*/

    if(flag_anchor){
        if(simplest_position == 0) strcat(R_mid, "^");
        else strcat(R_pre, "^");
    }

    for(int i = 0; i < children.size(); i++){
        if(i < simplest_position) strcat(R_pre, children[i]->get_re_part());
        else if(i <= last_position) strcat(R_mid, children[i]->get_re_part());
        else strcat(R_post, children[i]->get_re_part());
    }
}

void ComponentSequence::extract_non_one_repeat(char *R_pre, char *R_mid, char *R_post) {
    if(children.size() == 1) return;
    auto* compRep1 = (ComponentRepeat *) children[0];
    auto* compClass1 = (ComponentClass *) compRep1->sub_comp;

    if(typeid(*children[1]) == typeid(ComponentClass)){
        auto* compClass = (ComponentClass *) children[1];
        int cut = ceil(log(PMATCH_THRESHOLD/compClass->p_match()) / log(compClass1->p_match()));
        compRep1->save_value();
        compRep1->m_min -= cut;
        if(compRep1->m_max != _INFINITY) compRep1->m_max -= cut;
        strcpy(R_pre, compRep1->get_re_part());

        compRep1->recover_value();
        compRep1->m_min = compRep1->m_max = cut;
        strcat(R_mid, compRep1->get_re_part());
        strcat(R_mid, compClass->get_re_part());

        for(int i=2; i<children.size(); i++) strcat(R_post, children[i]->get_re_part());
    }
    else return ;
}
