//
// Created by 钟金诚 on 2020/8/8.
//

#include <sys/time.h>
#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "parser.h"
#include "Fb_DFA.h"

int VERBOSE;
int DEBUG;


/* usage */
static void usage()
{
    fprintf(stderr,"to do\n");
    exit(0);
}

/* configuration */
static struct conf {
    char *regex_file;
    char *trace_file;
    bool i_mod;
    bool m_mod;
    bool verbose;
    bool debug;
} config;

/* initialize the configuration */
void init_conf(){
    config.regex_file=NULL;
    config.trace_file=NULL;
    config.i_mod=false;
    config.m_mod=false;
    config.debug=false;
    config.verbose=false;
}

/* print the configuration */
void print_conf(){
    fprintf(stderr,"\nCONFIGURATION: \n");
    if (config.regex_file) fprintf(stderr, "- RegEx file: %s\n",config.regex_file);
    if (config.trace_file) fprintf(stderr,"- Trace file: %s\n",config.trace_file);
    if (config.i_mod) fprintf(stderr,"- ignore case selected\n");
    if (config.m_mod) fprintf(stderr,"- m modifier selected\n");
    if (config.verbose && !config.debug) fprintf(stderr,"- verbose mode\n");
    if (config.debug) fprintf(stderr,"- debug mode\n");
}

/* parse the main call parameters */
static int parse_arguments(int argc, char **argv)
{
    int i=1;
    if (argc < 2) {
        usage();
        return 0;
    }
    while(i<argc){
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
            usage();
            return 0;
        }else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0){
            config.verbose=1;
        }else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0){
            config.debug=1;
        }else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parse") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Regular expression file name missing.\n");
                return 0;
            }
            config.regex_file=argv[i];
        }else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Trace file name missing.\n");
                return 0;
            }
            config.trace_file=argv[i];
        }else if(strcmp(argv[i], "--m") == 0){
            config.m_mod=true;
        }else if(strcmp(argv[i], "--i") == 0){
            config.i_mod=true;
        }else{
            fprintf(stderr,"Ignoring invalid option %s\n",argv[i]);
        }
        i++;
    }
    return 1;
}

/* check that the given file can be read/written */
void check_file(char *filename, char *mode){
    FILE *file=fopen(filename,mode);
    if (file==NULL){
        fprintf(stderr,"Unable to open file %s in %c mode",filename,mode);
        fatal("\n");
    }else fclose(file);
}

int cmp(const void* p1, const void* p2){
    Fb_DFA *dfa1 = *(Fb_DFA **)p1;
    Fb_DFA *dfa2 = *(Fb_DFA **)p2;
    return dfa2->size() - dfa1->size();
}
/*
 * 1.first group largest dfa(with most dfa states)
 * 2.try to converge as many small dfa as possible
 * 3.guarantee worst run time
 * */
void greedy_group(list<Fb_DFA *> *dfa_lis){
    /*sort dfa according size*/
    Fb_DFA *tem_lis[5000];
    int cnt=0;
    for(list<Fb_DFA*>::iterator it = dfa_lis->begin(); it != dfa_lis->end(); it++) tem_lis[cnt++] = *it;
    qsort(tem_lis, cnt, sizeof(Fb_DFA*), cmp);
    //debug
    //for(int i=0; i<cnt; i++) printf("%d ", tem_lis[i]->size());
    //printf("\n");

    Fb_DFA* target_dfas[5000];/*save generated DFA*/
    memset(target_dfas, 0, sizeof(Fb_DFA*) * 5000);
    int res_cnt = 0;
    int beg = 0, end = cnt;
    for(int i = beg; i < end; i++){
        Fb_DFA* dfa = tem_lis[i];
        if(dfa == NULL) continue;/*have been converged*/
        /*ensure dfa can be put into FPGA alone*/
        if(!dfa->small_enough()){
            printf("dfa #%d cannot be put into FPGA alone!\n", i);
            delete dfa;
            continue;
        }
        /*converge small dfa first*/
        for(int j = end - 1; j > i; j--){
            if(tem_lis[j] == NULL) continue;
            Fb_DFA* dfa2 = tem_lis[j];
            Fb_DFA* new_dfa = dfa->converge(dfa2);
            if(new_dfa != NULL){
                delete dfa2;
                tem_lis[j] = NULL;
                delete dfa;
                dfa = new_dfa;
            }
        }
        target_dfas[res_cnt++] = dfa;
    }

    printf("target dfa number:%d\n", res_cnt);
    /*for(int i = 0; i < res_cnt; i++) {
        printf("#%d dfa size: %d\n", i, target_dfas[i]->size());
    }*/
}

list<char *>* read_regexset(char* fname){
    FILE* file = fopen(fname, "r");
    auto regex_list = new list<char *>();
    char tmp[1000];
    while(1){
        if(fgets(tmp, 1000, file)==NULL) break;
        if(tmp[0]=='#') continue;
        int len=strlen(tmp);
        if(len == 1 && tmp[len-1] == '\n') continue; //jump empty line
        if(tmp[len-1] == '\n')
        {
            tmp[len-1]='\0';
        }
        char *re= allocate_char_array(1000);
        strcpy(re, tmp);
        regex_list->push_back(re);
    }
    fclose(file);
    return regex_list;
}

/*
 * ptr前进一个concatenation，
 * 同时判定一条regex是否发生状态膨胀（发生一定程度T的状态膨胀）
 * */
#define EXPLOSION_REP_CHARSET 5
#define EXPLOSION_REP_CHAR 50
#define EXPLOSION_N_CONCAT 100
bool concatenation_blow(int &ptr, char* re, int &n_concat, bool flag_anchor, bool &flag_last_concat_star){
    int ptr_ = ptr;
    int i;
    int left = 1; //used in '('
    int ptr_recur = 0; // used in '('
    char re_recur[1000]; // used in '('
    int n_concat_recur = 0; // used in '('
    memset(re_recur, 0, 1000);

    bool flag_charset = false;

    //get one concatenation
    switch(re[ptr]){
        case '(':/*1.group (){}*/
            //get index of corresponding ')'
            for(i = ptr + 1; i < strlen(re); i++){
                if(re[i] == '(' && re[i-1] != '\\'){
                    left++;
                }
                if(re[i] == ')' && re[i-1] != '\\'){
                    left--;
                    if(left == 0) break;
                }
            }
            memcpy(re_recur, re + ptr + 1, i - ptr - 1);
            while(ptr_recur < i - ptr - 1){
                if(concatenation_blow(ptr_recur, re_recur, n_concat_recur, flag_anchor, flag_last_concat_star))
                {
                    ptr = ptr_;
                    return true;
                }
            }
            ptr = i + 1;
            flag_charset = true;
            break;
        case '[':/*2.charset []{}*/
            //get index of corresponding ']'
            for(i = ptr + 1; i < strlen(re); i++){
                if(re[i] == ']' && re[i-1] != '\\') break;
            }
            ptr = i + 1;
            flag_charset = true;
            break;
        case '.':
            ptr++;
            flag_charset = true;
            break;
        case '\\':
            ptr++;
            if(re[ptr] == 'w' || re[ptr] == 'W' || re[ptr] == 's' || re[ptr] == 'S' || re[ptr] == 'd' || re[ptr] == 'D'){
                ptr++;
                flag_charset = true;
            }
            else{
                if(re[ptr] == 'x' || re[ptr] == 'X') ptr += 3;
                else ptr += 1;
            }
            break;
        case '|':
            ptr++;
            return false;
            break;
        default:
            ptr++;
            break;
    }

    //get repetition
    int n = 1, m = _INFINITY;
    bool flag_current_concat_star = false;
    if(re[ptr] == STAR || re[ptr] == OPT || re[ptr] == PLUS){
        if(re[ptr] == STAR || re[ptr] == PLUS) flag_current_concat_star = true;
        ptr++;
    }
    else if(re[ptr] == '{'){
        ptr++;
        if(sscanf(re+ptr, "%d", &n) != 1) fatal("quantifier: wrong quantified expression.");
        while(ptr != strlen(re) && re[ptr] != COMMA && re[ptr] != CLOSE_QBRACKET)
            ptr++;
        if (ptr == strlen(re) || (re[ptr] == COMMA && ptr == strlen(re)-1))
            fatal("process_quantifier: wrong quantified expression.");
        if(re[ptr]==CLOSE_QBRACKET){
            m=n;
        }else{
            ptr++;
            if(re[ptr]!=CLOSE_QBRACKET){
                if (sscanf(re+ptr,"%d}",&m) != 1) fatal("regex_parser:: process_quantifier: wrong quantified expression.");
            }
        }
        while(re[ptr]!=CLOSE_QBRACKET)
            ptr++;
        ptr++;
    }

    n = max(n, m);//m not be _INF

    int n_concat_ = n_concat; //to roll back
    if(n_concat_recur) n_concat += (n * n_concat_recur);
    else n_concat += n;

    if(flag_anchor){
        if(n_concat > EXPLOSION_N_CONCAT || (flag_charset && flag_last_concat_star && n > EXPLOSION_REP_CHARSET)){
            ptr = ptr_;
            n_concat = n_concat_;
            return true;
        }
    }
    else if(n_concat > EXPLOSION_N_CONCAT || n > EXPLOSION_REP_CHAR || (flag_charset && n > EXPLOSION_REP_CHARSET)) {
        ptr = ptr_;
        n_concat = n_concat_;
        return true;
    }

    flag_last_concat_star = flag_current_concat_star;
    return false;
}

bitset<256> range_to_bs(int_set *range){
    bitset<256> bs;
    for(int i = 0; i < CSIZE; i++){
        if(range->item[i]) bs.set(i);
    }
    return bs;
}

bitset<256> get_charset(int &ptr, char *re){
    ptr++;
    if (re[ptr]==CLOSE_SBRACKET)
        fatal("regex_parser:: process_range: empty range.");
    bool negate=false;
    int from=CSIZE+1;
    int_set *range=new int_set(CSIZE);
    if(re[ptr]==TILDE){
        negate=true;
        ptr++;
    }
    while(ptr!=strlen(re)-1 && re[ptr]!=CLOSE_SBRACKET){
        symbol_t to = re[ptr];
        //if (is_special(to) && to!=ESCAPE)
        //	fatal("regex_parser:: process_range: invalid character.");
        if (to==ESCAPE){
            int_set *chars=new int_set(CSIZE);
            ptr=regex_parser::process_escape(re,ptr+1,chars);
            to=chars->head();
            delete chars;
        }
        else
            ptr++;
        if (from==(CSIZE+1)) from=to;
        if (ptr!=strlen(re)-1 && re[ptr]==MINUS_RANGE){
            ptr++;
        }else{
            if (from>to)
                fatal("regex_parser:: process_range: invalid range.");
            for(symbol_t i=from;i<=to;i++){
                range->insert(i);
                if (i==255) break;
            }
            from=CSIZE+1;
        }
    }
    if (re[ptr]!=CLOSE_SBRACKET)
        fatal("regex_parser:: process_range: range not closed.");
    ptr++;
    if (negate) range->negate();
    bitset<256> res = range_to_bs(range);
    delete range;
    return res;
}
bitset<256> get_char(int &ptr, char *re){
    bitset<256> res;
    int_set *chars=new int_set(CSIZE);
    switch(re[ptr]){
        case '|':
        case '(':
            fatal("situation not considered!");
            break;
        case '[':
            res = get_charset(ptr, re);
            break;
        case '.':
            res.set();
            ptr++;
            break;
        case '\\':
            ptr = regex_parser::process_escape(re, ptr+1, chars);
            res = range_to_bs(chars);
            break;
        default:
            int c = re[ptr];
            res.set(c);
            ptr++;
            break;
    }
    return res;
}

void split_re_repetition(int ptr_, int ptr, char *re, char *re_pre, char *re_post, int max_split_n){
    //1.find quantifier
    int ptr_quantifier = ptr - 1;
    assert(re[ptr] == '{');
    //2.get n,m
    int n,m=_INFINITY;
    ptr = ptr_quantifier+2;
    if(sscanf(re+ptr, "%d", &n) != 1) fatal("quantifier: wrong quantified expression.");
    while(ptr != strlen(re) && re[ptr] != COMMA && re[ptr] != CLOSE_QBRACKET)
        ptr++;
    if (ptr == strlen(re) || (re[ptr] == COMMA && ptr == strlen(re)-1))
        fatal("process_quantifier: wrong quantified expression.");
    if(re[ptr]==CLOSE_QBRACKET){
        m=n;
    }else{
        ptr++;
        if(re[ptr]!=CLOSE_QBRACKET){
            if (sscanf(re+ptr,"%d}",&m) != 1) fatal("regex_parser:: process_quantifier: wrong quantified expression.");
        }
    }
    while(re[ptr]!=CLOSE_QBRACKET)
        ptr++;
    ptr++;
    //3.split
    if(max_split_n > n) max_split_n = n;
    char charset[1000];
    //memset(charset, 0, 1000);
    memcpy(charset, re + ptr_, ptr_quantifier - ptr_ + 1);
    charset[ptr_quantifier - ptr_ + 1] = '\0';
    char str1[1000], str2[1000];
    sprintf(str1, "%s{%d}", charset, max_split_n);
    memcpy(re_pre, re, ptr_);
    strcat(re_pre, str1);

    str2[0] = '\0';
    if(m != _INFINITY) m = m-max_split_n;
    n = n - max_split_n;
    switch(n){
        case 0:
            if(m == _INFINITY) sprintf(str2, "%s*", charset);
            else if(m != 0) sprintf(str2, "%s{0,%d}", charset, m);
            break;
        default:
            if(m == _INFINITY) sprintf(str2, "%s{%d,}", charset, n);
            else if(m == n) sprintf(str2, "%s{%d}", charset, n);
            else sprintf(str2, "%s{%d,%d}", charset, n, m);
    }
    strcpy(re_post, str2);
    memcpy(re_post + strlen(str2), re + ptr, strlen(re) - ptr);
}
/*
 * 当以OF（overlapping factor 即large repetition）为分解点，R_pre较小时，尝试扩大R_pre
 * */
#define MAX_SPLIT_CONTAIN 5
#define MAX_SPLIT_NOTCONTAIN 100
bool try_expand_pre(int ptr, char *re, char* re_pre, char* re_post, bool flag_anchor){
    if(ptr >= strlen(re)) return false;
    /*no sence*/
    if(re[ptr] == '.') return false;
    /*a little complex*/
    if(re[ptr] == '(') {
        //todo
        return false;
    }
    if(re[ptr] == '|') return false; //impossible case?
    /*OF 为 char set []*/
    //bitset<256> bs_charset = get_charset(ptr, re);
    int ptr_ = ptr;
    bitset<256> bs_charset = get_char(ptr, re);
    bitset<256> bs_char;
    int tem_ptr = 0;
    if(flag_anchor && ptr_ != 1) {
        tem_ptr = 1;//anchor rule start from 1
        bs_char = get_char(tem_ptr, re);
    }
    else if(ptr_ != 0) bs_char = get_char(tem_ptr, re);

    bool is_contain = (bs_charset & bs_char).any();
    if(is_contain) split_re_repetition(ptr_, ptr, re, re_pre, re_post, MAX_SPLIT_CONTAIN);
    else split_re_repetition(ptr_, ptr, re, re_pre, re_post, MAX_SPLIT_NOTCONTAIN);
    return true;
}

/*
 * 若发生状态膨胀（包含大repetition）将一个regex前后分解为两个子regex
 *
 * */
int decompose_re(char* re, char* &re1, char* &re2){
    int len = strlen(re);
    char re_pre[1000];
    char re_post[1000];
    memset(re_pre, 0, 1000);
    memset(re_post, 0, 1000);
    bool flag_anchor = false;
    bool flag_last_concat_star = false;

    if(re[0] == '^'){/*^开头regex特殊处理，发生状态膨胀原因不同*/
        //todo
        //return 0;
        flag_anchor = true;
    }

    /*非^开头regex*/
    int ptr = 0; //pointer to re
    int n_concat = 0; //number of current concatenations (corresponding to ptr)
    while(ptr < len){//check by one concatenation after another
        if(concatenation_blow(ptr, re, n_concat, flag_anchor, flag_last_concat_star))
        {
            if(n_concat <= 2 && try_expand_pre(ptr, re, re_pre, re_post, flag_anchor)) break;
            else{
                memcpy(re_pre, re, ptr);
                memcpy(re_post, re+ptr, len-ptr);
            }
            break;
        }
    }

    if(strlen(re_post) == 0) return 0;
    //copy to return
    re1 = (char*) malloc(1000);
    re2 = (char*) malloc(1000);
    strcpy(re1, re_pre);
    strcpy(re2, re_post);
    return 1;
}
/*
 * 分析规则，将不能编译入一个 BRAM（|D|<=256 && |S|<=137）的规则分解为R_pre与R_post
 * input:原规则集
 * output:放入BRAM中的规则集S1 (R_pres + R_smalls)；
 *        放入CPU实现的规则集S2（R_posts）
 * */
int decompose_set(list<char *> *regex_list, list<char*>* &regex_s1, list<char*>* &regex_s2){
    FILE* file = fopen("../res/decompose.txt", "w");
    FILE* file2 = fopen("../res/decompose_bad.txt", "w");
    while(!regex_list->empty()){
        char *re = regex_list->front();
        regex_list->pop_front();
        char *re1 = NULL;
        char *re2 = NULL;
        int ret = decompose_re(re, re1, re2);
        if(ret){//实际发生分解
            //抛弃部分"bad"规则
            if(strlen(re1) <= 2){
                fprintf(file2, "*****     origin regex:%s\n%s\n%s\n\n", re, re1, re2);
                free(re1);
                free(re2);
                continue;
            }
            regex_s1->push_back(re1);
            regex_s2->push_back(re2);
            //fprintf(stderr, "*****     origin regex:%s\n%s\n%s\n\n", re, re1, re2);
            fprintf(file, "*****     origin regex:%s\n%s\n%s\n\n", re, re1, re2);
            free(re);
        }
        else{//未分解
            if(re1 != NULL) free(re1);
            if(re2 != NULL) free(re2);
            regex_s1->push_back(re);
        }
    }
    fclose(file);
    fclose(file2);
    return 0;
}


/*
 * re生成的Fb_DFA不满足small_enough要求，继续分解调整大小
 * */
bool try_decompose_re_further(char* re, Fb_DFA* fdfa){
    //todo
    return false;
    regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);
    auto nfa = parse->parse_from_regex(re);
    int over_states_number = fdfa->size() - 256;
    int over_large_state_number = fdfa->get_large_states_num() - 137;
    while(over_states_number > 0 || over_large_state_number>0){
        float over_ratio, r1, r2;
        r1 = (over_states_number - 256) * 1.0 / 256;
        r2 = (over_large_state_number - 137) * 1.0 / 137;
        over_ratio = max(r1, r2);
        //decompose_re_ratio();
    }

    //free
    delete parse;
    delete nfa;
}
/*
 * 将regex list中的规则编译到能放置于FPGA的BRAMs中
 * */
void compile_front_end(list<char*> *regex_list){
    struct timeval gstart, gend;
    FILE* file = fopen("../res/debug_frontend0818.txt", "w");

    regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);
    printf("REs to NFAs ing...\n");
    gettimeofday(&gstart, NULL);
    list<NFA *>* nfa_list = parse->parse_to_list_from_regexlist(regex_list);
    gettimeofday(&gend, NULL);
    printf("REs to NFAs cost time:%d seconds\n", gend.tv_sec - gstart.tv_sec);
    delete parse;

    printf("NFAs to DFAs ing...\n");
    gettimeofday(&gstart, NULL);
    int size = nfa_list->size();
    list<Fb_DFA *> *fb_dfa_lis = new list<Fb_DFA *>();
    for(int i = 0; i < size; i++){
        struct timeval start, end;
        gettimeofday(&start, NULL);
        if(i % 100 == 0) printf("%d/%d\n", i, size);
        NFA* nfa = nfa_list->front();
        //printf("now processing re: %s\n", nfa->pattern);
        nfa_list->pop_front();
        //nfa->remove_epsilon(); //can be omitted? toverify todo
        //nfa->reduce();
        DFA* dfa=nfa->nfa2dfa();
        if (dfa==NULL)
        {
            printf("Max DFA size %ld exceeded during creation: the DFA was not generated\n",MAX_DFA_SIZE);
            fprintf(file, "### cannot generate DFA, regex:%s\n\n", nfa->pattern);
        }
        else {
            dfa->minimize();
            //printf("8-bit DFA size:%d\n", dfa->size());

            Fb_DFA* fdfa = new Fb_DFA(dfa);
            Fb_DFA* minimum_dfa = fdfa->minimise();
            delete fdfa;
            if(minimum_dfa == NULL)
            {
                fprintf(file, "*** fdfa NULL, regex:%s\n\n", nfa->pattern);
                goto CONTINUE;
            }

            //printf("4-bit DFA size:%d\n", minimum_dfa->size());
            int cnt_cons2 = minimum_dfa->cons2states();
            if(minimum_dfa->small_enough()) fb_dfa_lis->push_back(minimum_dfa);
            else{
                fprintf(file, "*** fdfa->size:%d, cnt_cons2:%d, regex:%s\n\n", minimum_dfa->size(), cnt_cons2, nfa->pattern);
                if(try_decompose_re_further(nfa->pattern, minimum_dfa))
                {
                    fprintf(file, "SOLVE!\n");
                }
            }
        }
CONTINUE:
        gettimeofday(&end, NULL);
        int cost_seconds = end.tv_sec - start.tv_sec;
        if(cost_seconds > 5) fprintf(file, "time-consuming regex:%s\n", nfa->pattern);
        delete nfa;
        if(dfa != NULL) delete dfa;
    }
    gettimeofday(&gend, NULL);
    printf("NFAs to DFAs cost time:%d seconds\n", gend.tv_sec - gstart.tv_sec);

    printf("DFAs grouping...\n");
    gettimeofday(&gstart, NULL);
    greedy_group(fb_dfa_lis);
    gettimeofday(&gend, NULL);
    printf("DFAs grouping cost time:%d seconds\n", gend.tv_sec - gstart.tv_sec);

    //free
    fclose(file);
    delete fb_dfa_lis;
}

/*
 * 将regex list中的规则编译到能放置于CPU中
 * */
list<DFA*> * compile_back_end(list<char*> *regex_list){
    struct timeval start, end;
    list<DFA*> *dfa_list = new list<DFA*>();
    regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);
    //add ^ to each regex
    for(list<char*>::iterator it = regex_list->begin(); it != regex_list->end(); it++){
        char* re = *it;
        char tmp[1000];
        sprintf(tmp, "^%s", re);
        tmp[strlen(re) + 1] = '\0';
        strcpy(re, tmp);
    }

    printf("REs to NFAs ing...\n");
    gettimeofday(&start, NULL);
    list<NFA *>* nfa_list = parse->parse_to_list_from_regexlist(regex_list);
    gettimeofday(&end, NULL);
    printf("REs to NFAs cost time:%d seconds\n", end.tv_sec - start.tv_sec);
    delete parse;

    printf("NFAs to DFAs ing...\n");
    gettimeofday(&start, NULL);
    int size = nfa_list->size();
    for(int i = 0; i < size; i++){
        if(i % 100 == 0) printf("%d/%d\n", i, size);
        NFA* nfa = nfa_list->front();
        //printf("now processing re: %s\n", nfa->pattern);
        nfa_list->pop_front();
        DFA* dfa=nfa->nfa2dfa();
        if (dfa==NULL)
        {
            printf("Max DFA size %ld exceeded during creation: the DFA was not generated\n",MAX_DFA_SIZE);
            printf("explosive re:%s\n", nfa->pattern);
        }
        else {
            dfa->minimize();
            dfa_list->push_back(dfa);
        }
        delete nfa;
        //if(dfa != NULL) delete dfa;
    }
    gettimeofday(&end, NULL);
    printf("NFAs to DFAs cost time:%d seconds\n", end.tv_sec - start.tv_sec);

    return dfa_list;
}

void block_re(list<char*> *re_list){
    list<char*> *block_list = read_regexset("../res/snort_block.re");
    int filter_num = 0;

    for(list<char*>::iterator it = re_list->begin(); it != re_list->end(); it++){
        char* str = *it;
        bool flag_exist = false;
        for(list<char*>::iterator it2 = block_list->begin(); it2 != block_list->end(); it2++){
            char tmp[1000];
            strcpy(tmp, *it2);
            tmp[strlen(*it)] = '\0';
            if(strcmp(*it, tmp) == 0){
                flag_exist = true;
                break;
            }
        }
        if(flag_exist){
            //printf("filter regex:%s\n", str);
            filter_num++;
            re_list->erase(it);
        }
    }
    printf("filter rules num:%d\n", filter_num);
}
/*
 *  MAIN - entry point
 */
int main(int argc, char **argv){
    struct timeval start, end;

    //read configuration
    init_conf();
    while(!parse_arguments(argc,argv)) usage();
    print_conf();
    VERBOSE=config.verbose;
    DEBUG=config.debug;
    if (DEBUG) VERBOSE=1;

    //check that it is possible to open the files
    if (config.regex_file!=NULL) check_file(config.regex_file,"r");
    if (config.trace_file!=NULL) check_file(config.trace_file,"r");

    // check that either a regex file or a DFA import file are given as input
    if (config.regex_file==NULL){
        fatal("No data file - please use either a regex file\n");
    }

    list<char *> *regex_list = read_regexset(config.regex_file);
    //过滤一些暂不支持，或者耗时/影响性能的规则
    block_re(regex_list);
    //R_pres + R_smalls
    list<char *> *regex_s1 = new list<char *>();
    //R_posts
    list<char *> *regex_s2 = new list<char *>();
    decompose_set(regex_list, regex_s1, regex_s2);

    gettimeofday(&start, NULL);
    compile_front_end(regex_s1);
    gettimeofday(&end, NULL);
    printf("compile_front cost time:%d seconds\n", end.tv_sec - start.tv_sec);
    gettimeofday(&start, NULL);
    compile_back_end(regex_s2);
    gettimeofday(&end, NULL);
    printf("compile_back_end cost time:%d seconds\n", end.tv_sec - start.tv_sec);
    return 0;
}