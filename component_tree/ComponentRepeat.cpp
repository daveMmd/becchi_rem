//
// Created by 钟金诚 on 2020/8/13.
//

#include "ComponentRepeat.h"
#include "../stdinc.h"

ComponentRepeat::ComponentRepeat(Component *sub, int lb, int ub) {
    sub_comp = sub;
    m_min = lb;
    m_max = ub;
}

int ComponentRepeat::num_concat() {
    int repeat;
    if(m_max != _INFINITY) repeat = max(m_max, m_min);
    else repeat = m_min;
    return sub_comp->num_concat() * repeat;
}

bool ComponentRepeat::decompose(double cur_pmatch, int &threshold, std::bitset<256> &alpha, char *R_pre, char *R_post, int& depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass, bool top) {
    if(threshold <= 0 || cur_pmatch < PMATCH_THRESHOLD) return true;

    char tmp_R_pre[1000], tmp_R_post[1000];
    tmp_R_pre[0] = '\0';
    tmp_R_post[0] = '\0';
    //sprintf(tmp_R_post, "^");
    bool flag_decompose = false;
    int j = max(m_min, m_max);
    int cut = -1;
    //consider the situation that sub_comp is char class
    if(typeid(*sub_comp) == typeid(ComponentClass)){
        int max_cut_forpmatch = 0xfffffff;
        if(PMATCH_THRESHOLD > 0 && cur_pmatch <= 1 && sub_comp->p_match() < 1) max_cut_forpmatch = ceil(log(PMATCH_THRESHOLD / cur_pmatch) / log(sub_comp->p_match()));
        std::bitset<256> &beta = ((ComponentClass*)sub_comp)->charReach;

        //alpha*beta{j}, when alpha < beta || alpha ^ beta && alpha^beta!=beta 发生O(j^2)状态膨胀
        if(last_infinite_charclass->any() && ((*last_infinite_charclass) & beta).any() && ((*last_infinite_charclass) & beta) != beta){
            if(j <= 1) threshold -= j;
            else{
                if(depth >= 3) cut = 1;//only cut one, explosion doesn't occur
                else cut = 5; //need tolerate some explosion, to decrease match probability
                flag_decompose = true;
            }
        }
        //simple judge. char class too small, explosion not occur
        else if(beta.count() < 2 || alpha.count() < 2 || (alpha&beta).none() \
        || (*first_charClass & beta).none()) {
            if(threshold - j < 0)
            {
                cut = threshold;
                flag_decompose = true;
            }
            threshold -= j;
        }
        //not (@ >= #) --> O(j*j)
        else if(j < 5){
            if(threshold - j*j < 0) {
                cut = 0;
                flag_decompose = true;
            }
            threshold -= (j * j);
        }
        //else explosion occur
        else
        {
            if(depth < 3) cut = 5;
            else cut = 0;
            flag_decompose = true;
            threshold = 0;
        }

        //check if reach decompose for pmatch
        //if(cut >= max_cut_forpmatch || j >= max_cut_forpmatch){
        if(cut >= max_cut_forpmatch || m_min >= max_cut_forpmatch){
            cut = max_cut_forpmatch;
            flag_decompose = true;
        }

        //update alpha
        if(m_max == _INFINITY){
            alpha = alpha|beta;
            if(m_min != 0) *last_infinite_charclass = beta;
            else *last_infinite_charclass = (*last_infinite_charclass)|beta;
            //else *last_infinite_charclass = (*last_infinite_charclass)|alpha;
        }
        else if((beta&(*last_infinite_charclass)).any()){ //update first_charClass
            *first_charClass = (*first_charClass)|beta;
        }

        if(m_max != _INFINITY && m_min != 0) last_infinite_charclass->reset();

        /*考虑.{0,n}ES导致的O(n|ES|)数量级的状态膨胀*/
        if(!flag_decompose && (beta.count() > 240) && ((m_max - m_min) > 100)){
            strcat(R_pre, get_re_part());
            return true;
        }
    }
    //(RE){m,n} situation
    else{
        cut = 0;
        for(int i=0; i < m_min; i++)
        {
            tmp_R_pre[0] = '\0';
            tmp_R_post[0] = '\0';
            //assist to process (repeation of alternation)'s decomposition
            if(top && typeid(*sub_comp) == typeid(ComponentAlternation)) {
                flag_decompose = sub_comp->decompose(cur_pmatch, threshold, alpha, tmp_R_pre, tmp_R_post, depth, first_charClass, last_infinite_charclass, top);
                if(flag_decompose && !lis_R_pre.empty()){
                    m_min -= 1;
                    if(m_max != _INFINITY) m_max -= 1;
                }
            }
            else flag_decompose = sub_comp->decompose(cur_pmatch, threshold, alpha, tmp_R_pre, tmp_R_post, depth, first_charClass, last_infinite_charclass);
            if(flag_decompose) goto OUT_IF;
            else cut++;
            //update cur_pmatch
            if(cur_pmatch <= 1) cur_pmatch = cur_pmatch * sub_comp->p_match();
        }

        for(int i=0; i < m_max-m_min; i++){
            char tmp_R_pre2[1000], tmp_R_post2[1000];
            tmp_R_pre2[0]= '\0'; tmp_R_post2[0] = '\0';
            flag_decompose = sub_comp->decompose(2, threshold, alpha, tmp_R_pre2, tmp_R_post2, depth, first_charClass, last_infinite_charclass);
            if(flag_decompose) goto OUT_IF;
        }
    }

OUT_IF:
    if(flag_decompose){
        /*判定一个特殊条件：即当前charclass匹配概率过大时，放弃将其包含于prefix中*/
        if(typeid(*sub_comp) == typeid(ComponentClass) && sub_comp->p_match() > 240.0/256){
            cut = 0;
        }

        int tem_min = m_min;
        int tem_max = m_max;
        cut = min(cut, m_min);

        m_min = cut; m_max = cut;
        strcat(R_pre, get_re_part());
        if(cut < tem_min) strcat(R_pre, tmp_R_pre);
        //strcpy(R_post, tmp_R_post);
        strcat(R_post, tmp_R_post);
        if(cut < tem_min && strlen(tmp_R_pre)) cut++;

        m_min = tem_min - cut;
        if(tem_max == _INFINITY) m_max = tem_max;
        else m_max = tem_max - cut;
        strcat(R_post, get_re_part());
        return true;
    }

    strcat(R_pre, get_re_part());

    //update depth
    depth += m_min * sub_comp->num_concat();
    return false;
}

char *ComponentRepeat::get_re_part() {
    char* sub_re = sub_comp->get_re_part();
    char tmp[1000];
    if(m_min == m_max){
        if(m_min == 0){
            strcpy(re_part, "");
            return re_part;
        }
        if(m_min == 1) sprintf(tmp, "");
        else sprintf(tmp, "{%d}", m_min);
    }
    else if(m_max == _INFINITY)
    {
        if(m_min == 0) sprintf(tmp, "*");
        else if(m_min == 1) sprintf(tmp, "+");
        else sprintf(tmp, "{%d,}", m_min);
    }
    else if(m_min == 0 && m_max == 1) sprintf(tmp, "?");
    else sprintf(tmp, "{%d,%d}", m_min, m_max);

    //if(typeid(*sub_comp) == typeid(ComponentClass)) sprintf(re_part, "%s%s", sub_re, tmp);
    //else sprintf(re_part, "(%s)%s", sub_re, tmp);
    if(typeid(*sub_comp) == typeid(ComponentSequence)) sprintf(re_part, "(%s)%s", sub_re, tmp);
    else sprintf(re_part, "%s%s", sub_re, tmp);

    return re_part;
}

ComponentRepeat::~ComponentRepeat() {
    delete sub_comp;
}

double ComponentRepeat::p_match() {
    return pow(sub_comp->p_match(), m_min);
}

char *ComponentRepeat::get_reverse_re() {
    return get_re_part();
}

bool ComponentRepeat::is_dotstar() {
    if(m_max == _INFINITY && typeid(*sub_comp) == typeid(ComponentClass)){
        if(((ComponentClass*)sub_comp)->p_match() > 240.0 / 256) return true;
    }
    return false;
}
