//
// Created by 钟金诚 on 2020/11/20.
//

#include "runtime_fpga.h"
#include <unordered_map>
#include <list>
#include "Fb_DFA.h"
#include "dfa.h"
#include "prefix_DFA.h"
#include "trace.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp> //序列化STL容器要导入
#include <fstream>
#include "parse_pcap.h"

using namespace std;
#define MAX_CHARS_PER_LINE 5000

char *trace_fname = nullptr;
unordered_map<uint32_t, list<uint16_t>*> *mapping_table;
vector<prefix_DFA*> vec_prefixdfa;
int DEBUG = 0;
int VERBOSE = 0;

void load_mapping(){
    FILE *f = fopen("./mapping.txt", "r");

    mapping_table = new unordered_map<uint32_t, list<uint16_t>*>();
    char buff[MAX_CHARS_PER_LINE];
    int line_cnt = 0;
    while(fgets(buff, MAX_CHARS_PER_LINE, f) != nullptr){
        //printf("line %d\n", line_cnt++);
        char delims[] = " ";
        char *res = strtok(buff, delims);
        int ele_cnt = 0;
        uint32_t key = -1;
        auto *lis = new list<uint16_t>();
        while(res != nullptr){
            uint16_t accept_prefix;
            if(ele_cnt){
                sscanf(buff, "%u", &accept_prefix);
                lis->push_back(accept_prefix);
            }
            else{
                sscanf(buff, "%u", &key);
            }
            //printf("ele:%d res:%s\n", ele_cnt++, res);
            //read next element
            res = strtok(nullptr, delims);
        }
        (*mapping_table)[key] = lis;
    }

    fclose(f);
}

void load_prefixDfas(){
    ifstream ifs("vec_prefix_dfas.bin");
    boost::archive::text_iarchive ia(ifs);
    ia >> vec_prefixdfa;
    ifs.close();

    //load dfas in prefix dfas
    for(auto &it: vec_prefixdfa){
        it->complete_re = const_cast<char *>(it->complete_re_string.c_str());
        it->re = const_cast<char *> (it->re_string.c_str());

        it->import_dfa();//not need to import in FPGA-CPU arch, can be used for matching result verification
        prefix_DFA* parentNode = it->parent_node;
        while(parentNode != nullptr) {
            parentNode->import_dfa();
            parentNode = parentNode->parent_node;
        }
        prefix_DFA* childNode = it->next_node;
        while(childNode != nullptr){
            childNode->import_dfa();
            childNode = childNode->next_node;
        }
    }
}

int prefix_DFA::gid = 0;

/*note:一个报文，多个prefix dfas
 *
 * */
bool process_one_prefix_match(prefix_match* p_prefixMatch){
    //todo
    /*locate which prefix Dfas*/
    uint32_t key = p_prefixMatch->DFA_id * 212 + p_prefixMatch->state;
    list<uint16_t>* prefix_dfa_ids = (*mapping_table)[key];

    //locate which package
    packet_t *packet = nullptr;//todo

    for(auto &dfa_id: *prefix_dfa_ids){
        prefix_DFA* prefixDfa = vec_prefixdfa[dfa_id];
        prefixDfa->match(packet, p_prefixMatch->offset);
    }

    //reset the prefixMatch flag, to show that it has been handled
}

/* 初始化 fpga，建立通讯通道
 * */
void init_fpga(){
    //todo
    return;
}

/*macros determined by FPGA implementation*/
#define BRAM_NUM_SINGLE_UNIT 60
#define UNIT_NUMBER 28
#define M20K_CAP 2560

void** read_stt(int &num){
    FILE* f = fopen("./fpga_stt.bin", "rb");
    uint8_t buff[M20K_CAP + 5];
    num = 0;
    void** stts = (void**) malloc(sizeof(void*) * BRAM_NUM_SINGLE_UNIT * UNIT_NUMBER);
    memset(stts, 0, sizeof(void*) * BRAM_NUM_SINGLE_UNIT * UNIT_NUMBER);

    while(fread(buff, M20K_CAP, 1, f) == 1){
        uint8_t* stt = (uint8_t*) malloc(sizeof(uint8_t) * M20K_CAP);
        memcpy(stt, buff, M20K_CAP);
        stts[num++] = stt;
    }

    fclose(f);
    return stts;
}

void write_to_fpga_bram(void* stt, int bram_logic_num){
    //todo
};

void load_fpga_stt(){
    //todo
    int stt_num;
    void** stts = read_stt(stt_num);
    printf("read stt_num: %d\n", stt_num);

    //write to FPGA
    int need_unit_num;
    if(stt_num % BRAM_NUM_SINGLE_UNIT  == 0) need_unit_num = stt_num / BRAM_NUM_SINGLE_UNIT;
    else need_unit_num = stt_num / BRAM_NUM_SINGLE_UNIT + 1;
    printf("one copy of stt require need_unit_num:%d\n", need_unit_num);

    int copy_num;
    //if(UNIT_NUMBER % need_unit_num == 0) copy_num = UNIT_NUMBER / need_unit_num;
    //else copy_num = UNIT_NUMBER / need_unit_num + 1;
    copy_num = UNIT_NUMBER / need_unit_num;

    printf("maximally contain copy num:%d\n", copy_num);

    for(int i = 0; i < stt_num; i++)
        for(int j = 0; j < copy_num; j++){
            write_to_fpga_bram(stts[i], i + j * need_unit_num * BRAM_NUM_SINGLE_UNIT);
        }

    //free
    for(int i=0; i<stt_num; i++) free(stts[i]);
    free(stts);
}

void *loop_process_prefix_matches(void *arg){
    //todo
    printf("hello, new thread!\n");
}

/*start a thread to process prefix matches
 *
 * */
void start_new_thread_process(){
    pthread_t tid;
    if(pthread_create(&tid, nullptr, &loop_process_prefix_matches, nullptr) != 0){
        fprintf(stderr, "can not create new thread!\n");
        exit(-1);
    }
}

void loop_send_packages(){
    //todo
}

void test_using_software(){
    list<prefix_DFA*> lis_prefixdfa;
    lis_prefixdfa.insert(lis_prefixdfa.begin(), vec_prefixdfa.begin(), vec_prefixdfa.end());

    if (trace_fname){
        printf("simulating trace traverse, trace file:%s\n", trace_fname);
        trace::traverse_pcap(&lis_prefixdfa, trace_fname);

        /*int pkt_num = read_pcap(trace_fname);
        printf("pkt num:%d\n", pkt_num);
        for(int i=0; i<pkt_num; i++){
            printf("traversing %d/%d pkt...\n", i, pkt_num);
            trace::traverse(&lis_prefixdfa, pkts[i]->len, pkts[i]->content);
        }*/
        //auto tr=trace(trace_fname);
        //tr.traverse(&lis_prefixdfa);
    }
}

void verify_prefix_matches(){
    list<prefix_DFA*> lis_prefixdfa;
    lis_prefixdfa.insert(lis_prefixdfa.begin(), vec_prefixdfa.begin(), vec_prefixdfa.end());

    if (trace_fname){
        printf("verifying prefix matches using trace file:%s\n", trace_fname);
        trace::get_prefix_matches(&lis_prefixdfa, trace_fname, "prefixMatches.txt");
        //delete tr;
    }
}

int main(int argc, char **argv){

    //config
    if(argc >= 2) trace_fname = argv[1];

    //init
    init_fpga();

    /*loading compiled database*/
    load_mapping();
    load_prefixDfas();
    //test the correctness of loading databases
    //test_using_software();

    verify_prefix_matches();
    //exit(0);

    /*load front-end DFAs's STT into FPGA*/
    load_fpga_stt();

    /*start a thread to process prefix matches returned by FPGA*/
    start_new_thread_process();

    /*main thread responsible for sending packages to FPGA*/
    loop_send_packages();

    //sleep(5);
    return 0;
}