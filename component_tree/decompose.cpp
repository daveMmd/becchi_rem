//
// Created by 钟金诚 on 2020/8/13.
//
#include "decompose.h"
#define _INFILITY -1
/*
 * several main cases:
 * 1. /AB.{j}/  --> O(k+2^j)
 *    /@*AB#{j}/ --> O(k+2^j) 注：1）@(假)包含#，且#为字符集（>=两个字符）2）#(真)包含A(首字符)
 *    /@*ES1#{j}/ 注：1）|ES1|为0时不发生膨胀 2)|ES1|增大将减小规则膨胀，大致O(2^j / |ES1|^2)
 *
 * 2. /^A*[A-Z]{j}/ --> O(k+j^2)
 *    /^@*#{j}/ --> O(k+j^2) 注: 1)@(真)包含于#
 *    /^@*AB#{j}/ --> O(k+j^2) 注: 1)@(真)包含于# 2)AB(假)包含于@
 * */

/*
 * return p_match of R_pre
 * */
double decompose(char *re, char *R_pre, char *R_post, int threshold) {
    //init R_pre and R_post
    R_pre[0] = '\0';
    sprintf(R_post, "^");

    bool flag_anchor = false;
    if(re[0] == '^') {
        flag_anchor = true;
        sprintf(R_pre, "^");
    }
    Component* comp_tree = parse(re);

    std::bitset<256> alpha;
    //initialize @ to all zero(empty char set)
    if(flag_anchor) alpha.reset();
    else alpha.set(); //.* case

    int depth = 0;
    std::bitset<256> first_charClass;
    std::bitset<256> last_infinite_charclass;
    first_charClass.reset();
    if(flag_anchor) last_infinite_charclass.reset();
    else last_infinite_charclass.set();
    comp_tree->decompose(threshold, alpha, R_pre, R_post, depth, &first_charClass, &last_infinite_charclass);
    delete comp_tree;
    //failed to decompose
    if(strlen(R_pre) == 0 || strcmp(R_pre, "^") == 0) return 1.0;

    Component* comp = parse(R_pre);
    //int n_concat_pre = comp->num_concat();
    double p_match = comp->p_match();
    //process the case R_pre end up with .*
    if(typeid(*comp) == typeid(ComponentSequence)){
        int vec_size = ((ComponentSequence*)comp)->children.size();
        Component* last = ((ComponentSequence*)comp)->children.at(vec_size-1);
        if(typeid(*last) == typeid(ComponentRepeat)){
            Component* sub = ((ComponentRepeat*)last)->sub_comp;
            if(((ComponentRepeat*)last)->m_max == _INFILITY && typeid(*sub) == typeid(ComponentClass)){
                if(((ComponentClass*)sub)->charReach.all()){
                    R_pre[0] = '\0';
                    for(int i=0; i < vec_size - 1; i++){
                        strcat(R_pre, ((ComponentSequence*)comp)->children.at(i)->get_re_part());
                    }
                    if(strlen(R_post) > 1) {
                        char tmp[1000];
                        ((ComponentRepeat*)last)->m_max = ((ComponentRepeat*)last)->m_min;
                        sprintf(tmp, "%s%s", ((ComponentSequence*)comp)->children.at(vec_size - 1)->get_re_part(), R_post+1);
                        strcpy(R_post, tmp);
                    }
                }
            }
        }
    }

    delete comp;

    //return n_concat_pre;
    return p_match;
}

/*
 * 分解规则中匹配概率最低的长度确定的子部分
 * 注意：同时避免状态膨胀
 * return p_match of R_mid
 * */
double extract(char *re, char *R_pre, char* R_middle, char *R_post, int &depth, int threshold){
    //init R_pre, R_mid, R_post
    R_pre[0] = '\0';
    R_middle[0] = '\0';
    R_post[0] = '\0';

    //case 1. 假设从顶层sequence中提取,仅考虑class, class's repeat, sequence, sequence's repeat.
    Component* comp_tree = parse(re);
    if(typeid(*comp_tree) == typeid(ComponentSequence)){
        ((ComponentSequence*) comp_tree)->extract(R_pre, R_middle, R_post, threshold);
        delete comp_tree;

        if(strlen(R_middle) > 0){
            Component* comp = parse(R_middle);
            double p_match = comp->p_match();
            depth = comp->num_concat();
            delete comp;
            return p_match;
        }
    }

    return 1.0;
}

void reverse_re(char *re) {
    bool flag_anchor = false, flag_tail = false;
    if(re[0] == '^') flag_anchor = true;
    if(re[strlen(re) - 1] == '$') flag_tail = true;

    Component* comp = parse(re);
    char* re_reverse = comp->get_reverse_re();
    strcpy(re, re_reverse);
    delete comp;

    if(flag_anchor) strcat(re, "$");
    if(flag_tail) re[0] = '^';
}
