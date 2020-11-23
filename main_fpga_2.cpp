//
// Created by 钟金诚 on 2020/8/17.
//

#include <ctime>
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
    char *trace_base;
    bool i_mod;
    bool m_mod;
    bool verbose;
    bool debug;
} config;

/* initialize the configuration */
void init_conf(){
    config.regex_file=NULL;
    config.trace_file=NULL;
    config.trace_base = nullptr;
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
        }
        else if(strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--tracebase") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Trace base missing.\n");
                return 0;
            }
            config.trace_base=argv[i];
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
        fprintf(stderr,"Unable to open file %s in %s mode",filename,mode);
        fatal((char*)"\n");
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
Fb_DFA** greedy_group(list<Fb_DFA *> *dfa_lis, int& fbdfa_num){
    /*sort dfa according size*/
    Fb_DFA *tem_lis[10000];
    int cnt=0;
    for(list<Fb_DFA*>::iterator it = dfa_lis->begin(); it != dfa_lis->end(); it++) tem_lis[cnt++] = *it;
    qsort(tem_lis, cnt, sizeof(Fb_DFA*), cmp);

    Fb_DFA** target_dfas = (Fb_DFA**) malloc(sizeof(Fb_DFA*) * 5000);//5000];/*save generated DFA*/
    memset(target_dfas, 0, sizeof(Fb_DFA*) * 5000);
    int res_cnt = 0;
    int beg = 0, end = cnt;
    for(int i = beg; i < end; i++){
        //printf("processing %d/%d fb_dfa.\n", i, cnt);
        Fb_DFA* dfa = tem_lis[i];
        if(dfa == nullptr) continue;/*have been converged*/
        /*ensure dfa can be put into FPGA alone*/
        if(!dfa->small_enough()){
            printf("dfa #%d cannot be put into FPGA alone!\n", i);
            //delete dfa;
            continue;
        }
        /*converge large dfa first*/
        for(int j = i + 1; j < end; j++){
            if(tem_lis[j] == nullptr) continue;
            Fb_DFA* dfa2 = tem_lis[j];
            Fb_DFA* new_dfa = dfa->converge(dfa2);
            if(new_dfa != nullptr){
                //delete dfa2;
                tem_lis[j] = nullptr;
                //delete dfa;
                dfa = new_dfa;
            }
        }
        target_dfas[res_cnt++] = dfa;
    }

    fbdfa_num = res_cnt;

    printf("target dfa number(#brams):%d\n", res_cnt);
    int merged_fbstates_num = 0;
    for(int i = 0; i < res_cnt; i++) {
        //printf("#%d dfa size: %d\n", i, target_dfas[i]->size());
        merged_fbstates_num += target_dfas[i]->size();
    }
    printf("#merged fbstates num:%d\n", merged_fbstates_num);

    return target_dfas;
}

void block_re(list<char*> *re_list){
    list<char*> *block_list = read_regexset("../res/snort_block2.re");
    int filter_num = 0;

    for(list<char*>::iterator it = re_list->begin(); it != re_list->end(); it++){
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
    if(dfa->size() > MAX_STATES_NUMBER/2){
        float times = (dfa->size() - MAX_STATES_NUMBER/2) * 1.0 / (MAX_STATES_NUMBER/2);
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
        decompose(re_post, R_pre, R_post, PREFIX_DFA_STATES_THRESHOLD, false,true);
        if(strcmp(R_pre, "^") == 0 || strlen(R_pre) == 0){
            printf("in compile_prefixDFA() bad re:%s\n", re_post);
            delete prefixDfa;
            prefixDfa = nullptr;
            return prefixDfa;
        }
        NFA* nfa = parser.parse_from_regex(R_pre);
        DFA* dfa = nfa->nfa2dfa();
        if(dfa == nullptr) fatal((char*)"dfa NULL\n");
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
            fbdfa = nullptr;
            dfa = nullptr;
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
    int decompose_cnt = 0;

    FILE* file_multi_decompose = fopen("../res/multi_decompose.txt", "w");
    FILE* file_debug = fopen("../res/decompose_res.txt", "w");
    FILE* file_decompose_rules = fopen("../ruleset/snort_decompose.re", "w");
    for(auto &it: *regex_list){
        printf("processing %d/%d re:%s\n", ++cnt, size, it);
        list<prefix_DFA*> *prefixDfa_lis = compile_single_to_lis(it);
        if(prefixDfa_lis == nullptr || prefixDfa_lis->empty()) {
            prefixDfa_lis = try_compile_single_mid(it);
            //printf("middle extract re:%s\n", it);
        }

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

        //write multi_decompose rules to file_multi_decompose
        if(prefixDfa_lis->size() > 1){
            fprintf(file_multi_decompose, "decomposed to %d prefixDFAs\n", prefixDfa_lis->size());
            fprintf(file_multi_decompose, "RE:%s\n\n", it);
        }

        //write decomposed rules to file_decompose_rules
        bool flag_once = false;
        for(auto &it_prefixDfa: *prefixDfa_lis){
            if(it_prefixDfa->next_node == nullptr && it_prefixDfa->parent_node == nullptr) continue;
            if(flag_once) continue;
            fprintf(file_decompose_rules, "%s\n", it);
            flag_once = true;
            decompose_cnt++;
        }
    }

    printf("#bad rules: %d, #decompose rules: %d\n", bad_cnt, decompose_cnt);

    fclose(file_multi_decompose);
    fclose(file_debug);
    fclose(file_decompose_rules);
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

    if (config.trace_file){
        printf("simulating trace traverse, trace file:%s\n", config.trace_file);
        auto tr=new trace(config.trace_file);
        tr->traverse(prefix_dfa_list);
        delete tr;
    }

    if(config.trace_base){
        printf("simulating multiple trace traverse, trace base:%s\n", config.trace_base);
        auto tr=new trace();
        tr->traverse_multiple(prefix_dfa_list, config.trace_base);
        delete tr;
    }
}

void print_statics(list<prefix_DFA*> *prefix_dfa_list){
    //#post dfa states
    int post_states_num = 0;
    for(auto &it: *prefix_dfa_list){
        prefix_DFA* node = it->next_node;
        while(node!=nullptr){
            post_states_num += node->prefix_dfa->size();
            node = node->next_node;
        }
        node = it->parent_node;
        while(node!=nullptr){
            post_states_num += node->prefix_dfa->size();
            node = node->next_node;
        }
    }
    printf("#post dfa states:%d\n", post_states_num);

    //#prefix dfa states && #prefix fb-dfa states
    int prefx_states_num = 0;
    int fb_states_num = 0;
    for(auto &it: *prefix_dfa_list) {
        prefx_states_num += it->prefix_dfa->size();
        fb_states_num += it->fbDfa->size();
    }
    printf("#prefix dfa states:%d\n", prefx_states_num);
    printf("#prefix fb-dfa states:%d\n", fb_states_num);

    //#
}

bool mycomp(prefix_DFA* const &prefixDfa1, prefix_DFA* const &prefixDfa2){
    return prefixDfa1->fbDfa->size() > prefixDfa2->fbDfa->size();
}
/*
 * print size info of each prefix dfa, help analyse memory cost
 * */
void debug_prefix_dfa_size(list<prefix_DFA*> *prefix_dfa_list){
    FILE* file = fopen("../res/prefix_dfa_size.txt", "w");
    list<prefix_DFA*> prefixDfa_lis;
    for(auto &it: *prefix_dfa_list) prefixDfa_lis.push_back(it);
    prefixDfa_lis.sort(mycomp);
    for(auto &it: prefixDfa_lis){
        fprintf(file, "complete re:%s\n", it->complete_re);
        fprintf(file, "prefix re:%s\n", it->re);
        fprintf(file, "total sates: %d, cons2states: %d\n\n", it->fbDfa->size(), it->fbDfa->cons2states());
    }

    fclose(file);
}

void count_dotstar(list<char *> *regex_list){
    int cnt = 0;
    for(auto& it: *regex_list){
        if(contain_dotstar(it)) cnt++;
    }
    printf("# regexes containing dot_star:%d\n", cnt);
}


/*generate a file to write to FPGA BRAMs*/
void generate_fpga_stt(Fb_DFA** fb_dfas, int fbdfa_num){
    FILE* f_stt_dump = fopen("./fpga_stt.bin", "wb");
    //mappint table, record the mapping relation between prefix_dfa and postfix_dfa
    /*auto** mapping_table = (uint16_t**) malloc(sizeof(uint16_t*) * fbdfa_num);
    for(int i=0; i<fbdfa_num; i++){
        mapping_table[i] = (uint16_t *) malloc(sizeof(uint16_t) * MAX_STATES_NUMBER);
    }*/
    auto mapping_table = new map<uint32_t, list<uint16_t>*>();

    for(uint16_t i=0; i<fbdfa_num; i++){
        //printf("generating %d/%d BRAM mem...\n", i, fbdfa_num);
        void* mem_stt = fb_dfas[i]->to_BRAM(i, mapping_table);
        fwrite(mem_stt, 20480/8, 1, f_stt_dump);
    }
    fclose(f_stt_dump);

    /*dump mapping table*/
    FILE* f_mapping = fopen("./mapping.txt", "w");
    for(auto it: *mapping_table){
        uint32_t key = it.first;
        list<uint16_t>* lis = it.second;
        fprintf(f_mapping, "%u", key);
        for(auto it2: *lis){
            fprintf(f_mapping," %u", it2);
        }
        fprintf(f_mapping, "\n");
    }
    fclose(f_mapping);
    delete mapping_table;

    printf("finsished generating fpga_stt.\n");
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
    if (config.regex_file!= nullptr) check_file(config.regex_file,(char*)"r");
    if (config.trace_file!= nullptr) check_file(config.trace_file,(char*)"r");

    // check that either a regex file or a DFA import file are given as input
    if (config.regex_file== nullptr){
        fatal((char*)"No data file - please use either a regex file\n");
    }

    list<char *> *regex_list = read_regexset(config.regex_file);
    //过滤一些暂不支持，或者耗时/影响性能的规则
    block_re(regex_list);

    gettimeofday(&start, nullptr);
    list<prefix_DFA*> *prefix_dfa_list = compile(regex_list);
    gettimeofday(&end, nullptr);
    printf("compile cost time(generating dfa):%f seconds\n", (end.tv_sec - start.tv_sec) + 0.000001 * (end.tv_usec - start.tv_usec));

#if 1
    debug_prefix_dfa_size(prefix_dfa_list);
#endif
#if 1
    //print statics
    print_statics(prefix_dfa_list);

    //group fb_dfas
    int exact_string_prefix_num = 0;
    int fstates_inBRAM = 0;
    int dfastates_inBRAM = 0;
    list<Fb_DFA*> *fbDFA_lis = new list<Fb_DFA*>();
    vector<prefix_DFA*> *vec_prefixDFA = new vector<prefix_DFA*>(); //used for matching
    uint16_t ind_prefixdfa = 0;
    for(auto &it: *prefix_dfa_list){
        //prefix regex is exact string, put in single external SRAM instead of internal BRAM
        if(is_exactString(it->re)){
            exact_string_prefix_num++;
            //control if put exact string in BRAM
            //continue;
        }
        fstates_inBRAM += it->fbDfa->size();
        dfastates_inBRAM += it->prefix_dfa->size();
        fbDFA_lis->push_back(it->fbDfa);
        it->fbDfa->ind_prefixdfa_single = ind_prefixdfa++;//实际匹配运行时使用
        it->fbDfa = nullptr; //在greedy_group()里释放，防止重释放

        vec_prefixDFA->push_back(it);
    }
    printf("exact string prefix num:%d\n", exact_string_prefix_num);
    printf("In BRAM, dfa states num: %d\n",dfastates_inBRAM);
    printf("In BRAM, four-bit states num: %d\n",fstates_inBRAM);
    gettimeofday(&start, nullptr);

    int fbdfa_num = 0;
    Fb_DFA** fbdfas = greedy_group(fbDFA_lis, fbdfa_num);
    delete fbDFA_lis;
    gettimeofday(&end, nullptr);
    printf("grouping fb_dfa cost time:%llf seconds\n", (end.tv_sec - start.tv_sec) + 0.000001 * (end.tv_usec - start.tv_usec));
    /*generate STT for fbdfas in FPGA*/
    generate_fpga_stt(fbdfas, fbdfa_num);
#endif

    //simulate to examine CPU burden
    simulate(prefix_dfa_list);
    return 0;
}

//this is fgpa_run branch