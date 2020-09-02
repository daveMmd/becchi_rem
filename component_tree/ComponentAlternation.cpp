//
// Created by 钟金诚 on 2020/8/13.
//

#include "ComponentAlternation.h"
#include "../stdinc.h"

void ComponentAlternation::add(Component *pComp) {
    children.push_back(pComp);
}

int ComponentAlternation::num_concat() {
    int num = 0;
    for(std::vector<Component*>::iterator it=children.begin(); it != children.end(); it++){
        num += (*it)->num_concat();
    }
    return num;
}

char *ComponentAlternation::get_re_part() {
    //re_part[0] = '\0';
    sprintf(re_part, "(");
    for(auto it = children.begin(); it != children.end(); it++){
        if(strlen(re_part) == 1) strcat(re_part, (*it)->get_re_part());
        else{
            strcat(re_part, "|");
            strcat(re_part, (*it)->get_re_part());
        }
    }
    strcat(re_part, ")");
    return re_part;
}

bool ComponentAlternation::decompose(int &threshold, std::bitset<256> &alpha, char *R_pre, char *R_post, int& depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass, bool top) {
    //todo
#if 0  //simple implementation
    for(std::vector<Component*>::iterator it = children.begin(); it != children.end(); it++){
        threshold -= (*it)->num_concat();
    }
    if(threshold < 0) return true;

    /*if(strlen(R_pre) != 0){
        sprintf(R_pre + strlen(R_pre), "(%s)", get_re_part());
    }
    else strcat(R_pre, get_re_part());*/
    sprintf(R_pre + strlen(R_pre), "(%s)", get_re_part());
    return false;
#else
    int _threshold = threshold;
    if(threshold <= 0) return true;
    bool flag_decompose = false;
    std::bitset<256> union_alpha = alpha;
    std::bitset<256> union_first_charClass = *first_charClass;
    std::bitset<256> union_last_infinite_charclass;
    union_last_infinite_charclass.reset();
    for(auto & it : children){
        int tmp_threshold = threshold;
        char tmp_R_pre[1000], tmp_R_post[1000];
        tmp_R_pre[0] = '\0'; tmp_R_post[0] = '\0';
        int tmp_depth = depth;
        std::bitset<256> tmp_alpha = alpha;
        std::bitset<256> tmp_first_charClass = *first_charClass;
        std::bitset<256> tmp_last_infinite_charclass = *last_infinite_charclass;
        flag_decompose = it->decompose(tmp_threshold, tmp_alpha, tmp_R_pre, tmp_R_post, tmp_depth, &tmp_first_charClass, &tmp_last_infinite_charclass);
        union_alpha = union_alpha | tmp_alpha;
        union_first_charClass = union_first_charClass | tmp_first_charClass;
        union_last_infinite_charclass = union_last_infinite_charclass | tmp_last_infinite_charclass;
        int diff = threshold - tmp_threshold;
        threshold -= diff;
        if(threshold < 0) flag_decompose = true;
        if(flag_decompose) break;
    }
    if(flag_decompose){
        // 前非.* 或非top alternation 或已进行一次alternation分解 --> 不进行altenation分解
        if(!top || !last_infinite_charclass->all() || !lis_R_pre.empty()){
            //sprintf(R_post + strlen(R_post), "(%s)", get_re_part());
            strcat(R_post, get_re_part());
            return true;
        }
        //前.*, try to further decompose and get multiple R_pres and multiple R_posts
        int tmp_threshold = _threshold;
        char R_pre_alter[1000];
        R_pre_alter[0] = '\0';
        for(auto it=children.begin(); it != children.end(); it++){
            char tmp_R_pre[1000], tmp_R_post[1000];
            tmp_R_pre[0] = '\0';
            tmp_R_post[0] = '\0';
            int tmp_depth = depth;
            std::bitset<256> tmp_alpha = alpha;
            std::bitset<256> tmp_first_charClass = *first_charClass;
            std::bitset<256> tmp_last_infinite_charclass = *last_infinite_charclass;
            flag_decompose = (*it)->decompose(tmp_threshold, tmp_alpha, tmp_R_pre, tmp_R_post, tmp_depth, &tmp_first_charClass, &tmp_last_infinite_charclass);
            if(flag_decompose){
                if(strlen(R_pre_alter)){ //multile substring conclict
                    char * s_pre = (char*) malloc(1000);
                    char * s_post = (char*) malloc(1000);
                    char tmp[1000];
                    sprintf(tmp, "(%s)", R_pre_alter);
                    strcpy(s_pre, tmp);
                    s_post[0] = '\0';
                    lis_R_pre.push_back(s_pre);
                    lis_R_post.push_back(s_post);
                    it--;//backwards
                }
                else{//single substring of alternation decompose
                    char * s_pre = (char*) malloc(1000);
                    char * s_post = (char*) malloc(1000);
                    strcpy(s_pre, tmp_R_pre);
                    strcpy(s_post, tmp_R_post);
                    lis_R_pre.push_back(s_pre);
                    lis_R_post.push_back(s_post);
                }
                tmp_threshold = _threshold;
                R_pre_alter[0] = '\0';
            }
            else{
                char tmp[1000];
                if(strlen(R_pre_alter)) sprintf(tmp, "|%s", tmp_R_pre);
                else sprintf(tmp, "%s", tmp_R_pre);
                strcat(R_pre_alter, tmp);
            }
        }
        //last not appended to lis_R_pre
        if(strlen(R_pre_alter)){
            char * s_pre = (char*) malloc(1000);
            char * s_post = (char*) malloc(1000);
            char tmp[1000];
            sprintf(tmp, "(%s)", R_pre_alter);
            strcpy(s_pre, tmp);
            s_post[0] = '\0';
            lis_R_pre.push_back(s_pre);
            lis_R_post.push_back(s_post);
        }
        return true;
    }

    //sprintf(R_pre + strlen(R_pre), "(%s)", get_re_part());
    strcat(R_pre, get_re_part());

    //update first_charClass && union_last_infinite_charclass
    *first_charClass = union_first_charClass;
    *last_infinite_charclass = union_last_infinite_charclass;

    //update depth
    int min_n_concat = 0xffff;
    for(auto& it:children) min_n_concat = min(min_n_concat, it->num_concat());
    depth += min_n_concat;

    //update alpha
    alpha = union_alpha;
    return false;
#endif
}

ComponentAlternation::~ComponentAlternation() {
    for(auto &it:children){
        delete it;
    }
}

double ComponentAlternation::p_match() {
    double p = 0;
    for(auto &it: children){
        p = max(p, it->p_match());
    }
    return p;
}

char *ComponentAlternation::get_reverse_re() {
    //re_part[0] = '\0';
    sprintf(re_part, "(");
    for(auto it = children.begin(); it != children.end(); it++){
        if(strlen(re_part) == 1) strcat(re_part, (*it)->get_reverse_re());
        else{
            strcat(re_part, "|");
            strcat(re_part, (*it)->get_reverse_re());
        }
    }
    strcat(re_part, ")");
    return re_part;
}
