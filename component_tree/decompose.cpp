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
std::list<char*> lis_R_pre; //used to save multiple R_pres
std::list<char*> lis_R_post; //used to save multiple R_posts (correspond to lis_R_pre)
double decompose(char *re, char *R_pre, char *R_post, int threshold, bool use_pmatch, bool control_top) {
    double p_match = 0;
    //init R_pre and R_post
    R_pre[0] = '\0';
    sprintf(R_post, "^");
    //init lis_R_pre and lis_R_post
    for(auto &it: lis_R_pre) free(it);
    for(auto &it: lis_R_post) free(it);
    lis_R_pre.clear();
    lis_R_post.clear();

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

    double cur_pmatch = 2;
    if(use_pmatch) cur_pmatch = 1;
    comp_tree->decompose(cur_pmatch, threshold, alpha, R_pre, R_post, depth, &first_charClass, &last_infinite_charclass, !control_top);
    //if(!control_top) comp_tree->decompose(cur_pmatch, threshold, alpha, R_pre, R_post, depth, &first_charClass, &last_infinite_charclass, true);
    //else comp_tree->decompose(cur_pmatch, threshold, alpha, R_pre, R_post, depth, &first_charClass, &last_infinite_charclass);
    delete comp_tree;

    /*try get lis_R_pre and lis_R_post*/
    if(!lis_R_pre.empty()){
        //printf("***lis_R_pre and lis_R_post***\n");
        auto it2 = lis_R_post.begin();
        for(auto it = lis_R_pre.begin(); it != lis_R_pre.end(); it++){
            char tmp[1000];
            sprintf(tmp, "%s%s", R_pre, *it);
            strcpy(*it, tmp);
            sprintf(tmp, "^%s%s", *it2, R_post + 1);
            strcpy(*it2, tmp);
            it2++;
            //caculate p_match
            Component* comp = parse(*it);
            p_match = std::max(p_match, comp->p_match());
            delete comp;
        }
        return p_match;
    }

    //failed to decompose
    if(strlen(R_pre) == 0 || strcmp(R_pre, "^") == 0) return 1.0;

    Component* comp = parse(R_pre);
    //int n_concat_pre = comp->num_concat();
    p_match = comp->p_match();
#if 0 //if successfully process () and |, the case R_pre end up with .* will (probably) not occur
    //process the case R_pre end up with .*
    if(strlen(R_post) > 1 && typeid(*comp) == typeid(ComponentSequence)){
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
                    //if(strlen(R_post) > 1) {
                        char tmp[1000];
                        ((ComponentRepeat*)last)->m_max = ((ComponentRepeat*)last)->m_min;
                        sprintf(tmp, "%s%s", ((ComponentSequence*)comp)->children.at(vec_size - 1)->get_re_part(), R_post+1);
                        strcpy(R_post, tmp);
                    //}
                }
            }
        }
    }
#endif

    delete comp;

    if(flag_anchor) p_match *= 0.001; //inclined to use anchor R_pre
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

bool is_exactString(char *re){
    Component* comp = parse(re);
    ComponentSequence *compSeq;
    ComponentClass *compClass;
    if(typeid(*comp) != typeid(ComponentSequence)) goto FALSE;
    compSeq = (ComponentSequence *) comp;
    for(auto &it: compSeq->children){
        if(typeid(*it) != typeid(ComponentClass)) goto FALSE;
        compClass = (ComponentClass *) it;
        if(compClass->charReach.count() != 1) goto FALSE;
    }
    delete comp;
    return true;

    FALSE:
    delete comp;
    return false;
}