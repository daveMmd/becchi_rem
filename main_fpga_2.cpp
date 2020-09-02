//
// Created by 钟金诚 on 2020/8/17.
//

#include <sys/time.h>
#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "./component_tree/decompose.h"
#include "./component_tree/util.h"
#include "Fb_DFA.h"
#include "parser.h"
#include "prefix_DFA.h"
#include "trace.h"

int VERBOSE;
int DEBUG;

/* usage */
static void usage()
{
    fprintf(stderr,"argument wrong. usage to do\n");
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

/*
 * 将regex list中的规则编译到能放置于FPGA的BRAMs中
 * */
void compile_front_end(list<char*> *regex_list){
    struct timeval gstart, gend;
    FILE* file = fopen("../res/debug_frontend.txt", "w");

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
        if(i % 1 == 0) printf("%d/%d\n", i, size);
        NFA* nfa = nfa_list->front();
        printf("now processing re: %s\n", nfa->pattern);
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
    FILE* file = fopen("../res/debug_backend.txt", "w");
    auto *dfa_list = new list<DFA*>();
    auto *parse=new regex_parser(config.i_mod,config.m_mod);

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
        struct timeval lstart, lend;
        gettimeofday(&lstart, NULL);

        if(i % 1 == 0) printf("%d/%d\n", i, size);
        NFA* nfa = nfa_list->front();
        printf("now processing re: %s\n", nfa->pattern);
        nfa_list->pop_front();
        DFA* dfa=nfa->nfa2dfa();
        if (dfa==NULL)
        {
            fprintf(file, "Max DFA size %ld exceeded during creation: the DFA was not generated\n",MAX_DFA_SIZE);
            fprintf(file, "explosive re:%s\n", nfa->pattern);
        }
        else {
            dfa->minimize();
            dfa_list->push_back(dfa);
        }

        gettimeofday(&lend, NULL);
        if(lend.tv_sec - lstart.tv_sec > 5){
            fprintf(file, "time-consuming (%d seconds) R_post:%s\n", lend.tv_sec - lstart.tv_sec, nfa->pattern);
        }
        delete nfa;
        //if(dfa != NULL) delete dfa;
    }
    gettimeofday(&end, NULL);
    printf("NFAs to DFAs cost time:%d seconds\n", end.tv_sec - start.tv_sec);

    fclose(file);
    return dfa_list;
}

void block_re(list<char*> *re_list){
    list<char*> *block_list = read_regexset("../res/snort_block2.re");
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
            it = re_list->erase(it);
            it--;
        }
    }
    printf("filter rules num:%d\n", filter_num);
}

void decompose_set(list<char*> *regex_list, list<char *> *regex_s1, list<char *> *regex_s2){
    int bad_cnt = 0;
    FILE* file = fopen("../res/bad_cannot_decompose.re", "w");
    for(auto &it: *regex_list){
        char R_pre[1000], R_post[1000];
        int n_concat_pre = decompose(it, R_pre, R_post);

        //bad rules
        if(n_concat_pre < 3 && strcmp(it, R_pre) != 0){
            //printf("bad rule %d:%s\n", ++bad_cnt, it);
            fprintf(file, "bad rule %d:%s\n", ++bad_cnt, it);
            //printf("R_pre:%s\n", R_pre);
            //printf("R_post:%s\n", R_post);
            continue;
        }
        //未发生分解
        if(strcmp(R_post, "^") == 0){
            regex_s1->push_back(it);
        }
        else{//发生分解
            char* s1 = (char*) malloc(1000);
            //char* s2 = (char*) malloc(1000);
            char s2[1000];
            strcpy(s1, R_pre); strcpy(s2, R_post);
            regex_s1->push_back(s1);
            //regex_s2->push_back(s2);
            //try recursively decompose R_post
            char tmp[10000];
            tmp[0] = '\0';
            bool flag_multi_decompose = false;
            sprintf(tmp + strlen(tmp), "%s\n", R_pre);
            while(1){
                decompose(s2, R_pre, R_post, 10000);
                if(strcmp(R_pre, "^") == 0){
                    printf("bad R_post:%s\n", s2);
                    break;
                }
                sprintf(tmp + strlen(tmp), "%s\n", R_pre);
                char* s2_pre = (char*) malloc(1000);
                strcpy(s2_pre, R_pre);
                regex_s2->push_back(s2_pre);
                if(strcmp(R_post, "^") == 0) break;
                flag_multi_decompose = true;
                strcpy(s2, R_post);
            }
            if(flag_multi_decompose != 0){
                printf("re:%s\n", it);
                printf("%s\n", tmp);
            }

            delete it;
        }
    }

    fclose(file);
}

float bram_compatible(char *re, Fb_DFA* &fbdfa, DFA* &ref_dfa){
    auto parser = regex_parser(config.i_mod, config.m_mod);
    NFA* nfa = nullptr;
    DFA* dfa = nullptr;
    Fb_DFA* fb_dfa = nullptr;
    Fb_DFA* minimise_fb_dfa = nullptr;
    float ratio = 1;

    nfa = parser.parse_from_regex(re);
    dfa = nfa->nfa2dfa();
    if(dfa == nullptr) goto RET;
    dfa->minimize();
    if(dfa->size() > 128){
        float times = (dfa->size() - 100) * 1.0 / 128;
        ratio = times / (times + 1);
        goto RET;
    }
    fb_dfa = new Fb_DFA(dfa);

    //minimise_fb_dfa = fb_dfa->minimise();
    minimise_fb_dfa = fb_dfa->minimise2();
    if(minimise_fb_dfa == nullptr) goto RET;
    ratio = minimise_fb_dfa->get_ratio();

    RET:
    delete nfa;
    //delete dfa;
    delete fb_dfa;

    if(ratio > 0) {
        delete minimise_fb_dfa;
        delete dfa;
    }
    else {
        fbdfa = minimise_fb_dfa;
        ref_dfa = dfa;
    }
    return ratio;
}

#define T_MATCH 0.0001
#define PREFIX_DFA_STATES_THRESHOLD 10000

prefix_DFA* compile_prefixDFA(char* R_post){
    //process empty R_post case
    if(strlen(R_post) <= 0 || strcmp(R_post, "^") == 0) return nullptr;
    auto *prefixDfa = new prefix_DFA();
    //get R_post(s) and DFA(s); try decompose R_post further
    auto parser = regex_parser(config.i_mod, config.m_mod);
    while(strlen(R_post) > 0 && strcmp(R_post, "^") != 0){
        char re_post[1000], R_pre[1000];//, R_post[1000];
        strcpy(re_post, R_post);
        decompose(re_post, R_pre, R_post, PREFIX_DFA_STATES_THRESHOLD, true);
        if(strcmp(R_pre, "^") == 0 || strlen(R_pre) == 0){
            printf("in compile_prefixDFA() bad re:%s\n", re_post);
            delete prefixDfa;
            prefixDfa = nullptr;
            return prefixDfa;
        }
        NFA* nfa = parser.parse_from_regex(R_pre);
        DFA* dfa = nfa->nfa2dfa();
        if(dfa == nullptr) fatal("dfa NULL\n");
        dfa->minimize();
        //add dfa to prefix_dfa
        prefixDfa->add_dfa(dfa);
        //prefixDfa->add_re(re_post);
        prefixDfa->add_re(R_pre);
        if(R_post[0] != '^') prefixDfa->set_activate_once();

        //free
        delete nfa;
    }

    return prefixDfa;
}

prefix_DFA* compile_single(char* re){
    Fb_DFA* fbdfa = nullptr;
    DFA* dfa = nullptr;
    prefix_DFA* prefixDfa = nullptr;
    char R_pre[1000], R_post[1000];

    //get R_pre and fbdfa
    double p_match = decompose(re, R_pre, R_post);
    if(p_match > T_MATCH && strcmp(re, R_pre) != 0){
        //todo 尝试提取规则其他部分
        printf("bad R_pre:%s\n", R_pre);
        return nullptr;
    }
    int threshold = DEAFAULT_THRESHOLD;
    float ratio = bram_compatible(R_pre, fbdfa, dfa);
    while(ratio > 0){
        threshold = (1-ratio) * threshold;
        p_match = decompose(re, R_pre, R_post, threshold);
        if(p_match > T_MATCH && strcmp(re, R_pre) != 0){
            //todo 尝试提取规则其他部分
            printf("bad R_pre:%s\n", R_pre);
            return nullptr;
        }
        ratio = bram_compatible(R_pre, fbdfa, dfa);
    }
    prefixDfa = new prefix_DFA();
    prefixDfa->add_dfa(dfa);
    //prefixDfa->add_re(re);
    prefixDfa->add_re(R_pre);
    //todo not used temporarily
    //delete fbdfa;
    prefixDfa->add_fbdfa(fbdfa);
    if(R_post[0] != '^') prefixDfa->set_activate_once(); //not '^', must be '.*'

    //get R_post(s) and DFA(s); try decompose R_post further
    auto parser = regex_parser(config.i_mod, config.m_mod);
    while(strcmp(R_post, "^") != 0){
        char re_post[1000];
        strcpy(re_post, R_post);
        decompose(re_post, R_pre, R_post, PREFIX_DFA_STATES_THRESHOLD, true);
        if(strcmp(R_pre, "^") == 0 || strlen(R_pre) == 0){
            printf("bad R_post:%s\n", re_post);
            //flag_badpost = true;
            delete prefixDfa;
            prefixDfa = nullptr;
            return prefixDfa;
        }
        NFA* nfa = parser.parse_from_regex(R_pre);
        DFA* dfa2 = nfa->nfa2dfa();
        if(dfa2 == nullptr) fatal("dfa NULL\n");
        dfa2->minimize();
        //add dfa to prefix_dfa
        prefixDfa->add_dfa(dfa2);
        //prefixDfa->add_re(re_post);
        prefixDfa->add_re(R_pre);
        if(R_post[0] != '^') prefixDfa->set_activate_once();

        //free
        delete nfa;
    }

    //add fdba and dfa(s) to up-level process
    return prefixDfa;
}

//extern std::list<char*> lis_R_pre; //used to save multiple R_pres
//extern std::list<char*> lis_R_post; //used to save multiple R_posts (correspond to lis_R_pre)
list<prefix_DFA*>* compile_single_to_lis(char* re){
    Fb_DFA* fbdfa = nullptr;
    DFA* dfa = nullptr;
    auto *lis_prefixDfa = new list<prefix_DFA*>();
    char R_pre[1000], R_post[1000];

    //get compatible R_pre(s)
    int threshold = DEAFAULT_THRESHOLD;
    while(1){
        double p_match = decompose(re, R_pre, R_post, threshold);
        if(p_match > T_MATCH && strcmp(re, R_pre) != 0){
            //todo 尝试提取规则其他部分
            printf("bad R_pre:%s\n", R_pre);
            return nullptr;
        }
        if(lis_R_pre.empty()) {
            char *tmp_pre = (char *) malloc(1000);
            char *tmp_post = (char *) malloc(1000);
            strcpy(tmp_pre, R_pre);
            strcpy(tmp_post, R_post);
            lis_R_pre.push_back(tmp_pre);
            lis_R_post.push_back(tmp_post);
        }
        float ratio = 0;
        for(auto &it: lis_R_pre){
            ratio = max(ratio, bram_compatible(it, fbdfa, dfa));
            auto *prefixDfa = new prefix_DFA();
            prefixDfa->add_dfa(dfa);
            prefixDfa->add_re(it);
            prefixDfa->add_fbdfa(fbdfa);
            lis_prefixDfa->push_back(prefixDfa);
        }
        if(ratio <= 0) break;
        for(auto &it: *lis_prefixDfa) delete it;
        lis_prefixDfa->clear();
        threshold = (1-ratio) * threshold;
    }

    list<char*> lis_R_post_copy;
    lis_R_post_copy.clear();
    for(auto &it :lis_R_post){
        char *tmp = (char*) malloc(1000);
        strcpy(tmp, it);
        lis_R_post_copy.push_back(tmp);
    }

    auto it_lis = lis_prefixDfa->begin();
    for(auto &it: lis_R_post_copy){
        prefix_DFA* prefixDfa = compile_prefixDFA(it);
        (*it_lis)->next_node = prefixDfa;
        it_lis++;
    }
    return lis_prefixDfa;
}

//try to extract middle re_part && compile
//prefix_DFA* try_compile_single_mid(char* re){
list<prefix_DFA*> * try_compile_single_mid(char* re){
    char R_pre[1000], R_mid[1000], R_post[1000];
    prefix_DFA* prefixDfa_post = nullptr;
    prefix_DFA* prefixDfa_pre = nullptr;
    prefix_DFA* prefixDfa = nullptr;
    auto *lis_prefixDfa = new list<prefix_DFA*>();
    int depth = 0;

    double p_match = extract(re, R_pre, R_mid, R_post, depth);
    if(p_match > T_MATCH) return nullptr;

    //first compile R_mid
    Fb_DFA* fbDfa = nullptr;
    DFA* dfa = nullptr;
    float ratio = bram_compatible(R_mid, fbDfa, dfa);
    if(ratio > 0) goto FAIL;

    //compile R_post
    if(strlen(R_post) > 0){
        char R_post_new[1000];
        //todo: judge .* and judge activate once?
        sprintf(R_post_new, "^%s", R_post);
        prefixDfa_post = compile_prefixDFA(R_post_new);
        if(prefixDfa_post == nullptr) goto FAIL;
    }


    //compile reverse R_pre to dfa
    if(strlen(R_pre) > 0){
        reverse_re(R_pre);
        char R_pre_new[1000];
        sprintf(R_pre_new, "^%s", R_pre);
        prefixDfa_pre = compile_prefixDFA(R_pre_new);
        if(prefixDfa_pre == nullptr) goto FAIL;
    }



    prefixDfa = new prefix_DFA();
    prefixDfa->depth = depth;
    prefixDfa->add_dfa(dfa);
    //prefixDfa->add_re(re);
    prefixDfa->add_re(R_mid);
    prefixDfa->add_fbdfa(fbDfa);
    prefixDfa->parent_node = prefixDfa_pre;
    prefixDfa->next_node = prefixDfa_post;
    lis_prefixDfa->push_back(prefixDfa);
    return lis_prefixDfa;

    FAIL:
    delete prefixDfa_pre;
    delete prefixDfa_post;
    delete fbDfa;
    delete dfa;
    delete lis_prefixDfa;
    return nullptr;
}

list<prefix_DFA*> *compile(list<char*> *regex_list){
    auto prefix_dfa_list = new list<prefix_DFA*>();
    int size = regex_list->size();
    int cnt = 0;
    int bad_cnt = 0;

    FILE* file_debug = fopen("../res/decompose_res.txt", "w");
    for(auto &it: *regex_list){
        printf("processing %d/%d re:%s\n", ++cnt, size, it);
        //prefix_DFA* prefixDfa = compile_single(it);
        list<prefix_DFA*> *prefixDfa_lis = compile_single_to_lis(it);
        //if(prefixDfa == nullptr) prefixDfa = try_compile_single_mid(it);
        if(prefixDfa_lis == nullptr || prefixDfa_lis->empty()) prefixDfa_lis = try_compile_single_mid(it);

        if(prefixDfa_lis == nullptr || prefixDfa_lis->empty()) {
            printf("BAD RULE-%d.\n", ++bad_cnt);
            fprintf(file_debug, "BAD RULE-%d:%s\n", bad_cnt, it);
            continue;
        }
        for(auto &it_prefixDfa: *prefixDfa_lis) it_prefixDfa->debug(file_debug, it);
        for(auto &it_prefixDfa: *prefixDfa_lis) {
            it_prefixDfa->complete_re = it;
            prefix_dfa_list->push_back(it_prefixDfa);
        }
    }

    fclose(file_debug);
    return prefix_dfa_list;
}

/*
 * simulate to examine CPU burden/overhead
 * */
void simulate(list<prefix_DFA*> *prefix_dfa_list){
    //first delete prefix_dfas that size == 1 (no overhead on cpu)
    int total_rule_num = 0;
    int benign_rule_num = 0;
    for(auto it =  prefix_dfa_list->begin(); it != prefix_dfa_list->end(); it++){
        total_rule_num++;
        if((*it)->next_node == nullptr && (*it)->parent_node == nullptr) {
            benign_rule_num++;
            delete *it;
            it = prefix_dfa_list->erase(it);
            it--;//need roll back?
        }
    }
    printf("total prefixDFA num:%d\n", total_rule_num);
    printf("benign prefixDFA num:%d\n", benign_rule_num);

    //todo
    if (config.trace_file){
        printf("simulating trace traverse, trace file:%s\n", config.trace_file);
        auto tr=new trace(config.trace_file);
        tr->traverse(prefix_dfa_list);
        delete tr;
    }
}

/*
 *  MAIN - entry point
 */
#if 0
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
    auto *regex_s1 = new list<char *>();
    //R_posts
    auto *regex_s2 = new list<char *>();
    decompose_set(regex_list, regex_s1, regex_s2);

#if 1
    gettimeofday(&start, NULL);
    compile_front_end(regex_s1);
    gettimeofday(&end, NULL);
    printf("compile_front cost time:%d seconds\n", end.tv_sec - start.tv_sec);
    gettimeofday(&start, NULL);
    compile_back_end(regex_s2);
    gettimeofday(&end, NULL);
    printf("compile_back_end cost time:%d seconds\n", end.tv_sec - start.tv_sec);
#endif
    return 0;
}
#else
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

    gettimeofday(&start, nullptr);
    list<prefix_DFA*> *prefix_dfa_list = compile(regex_list);
    gettimeofday(&end, nullptr);
    printf("compile cost time:%d seconds\n", end.tv_sec - start.tv_sec);

    //simulate to examine CPU burden
    simulate(prefix_dfa_list);
    return 0;
}
#endif