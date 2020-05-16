//
// Created by 钟金诚 on 2019/8/26.
//

/*
 * Copyright (c) 2007 Michela Becchi and Washington University in St. Louis.
 * All rights reserved
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. The name of the author or Washington University may not be used
 *       to endorse or promote products derived from this source code
 *       without specific prior written permission.
 *    4. Conditions of any other entities that contributed to this are also
 *       met. If a copyright notice is present from another entity, it must
 *       be maintained in redistributions of the source code.
 *
 * THIS INTELLECTUAL PROPERTY (WHICH MAY INCLUDE BUT IS NOT LIMITED TO SOFTWARE,
 * FIRMWARE, VHDL, etc) IS PROVIDED BY  THE AUTHOR AND WASHINGTON UNIVERSITY
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR WASHINGTON UNIVERSITY
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS INTELLECTUAL PROPERTY, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * */

/*
 * File:   main.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 *
 * Description: This is the main entry file
 *
 */

#include <vector>
#include <string>
#include <sys/time.h>
#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "ecdfa.h"
#include "hybrid_fa.h"
#include "parser.h"
#include "trace.h"
#include "dave_util.h"

/*
 * Program entry point.
 * Please modify the main() function to add custom code.
 * The options allow to create a DFA from a list of regular expressions.
 * If a single single DFA cannot be created because state explosion occurs, then a list of DFA
 * is generated (see MAX_DFA_SIZE in dfa.h).
 * Additionally, the DFA can be exported in proprietary format for later re-use, and be imported.
 * Moreover, export to DOT format (http://www.graphviz.org/) is possible.
 * Finally, processing a trace file is an option.
 */


#ifndef CUR_VER
#define CUR_VER		"Michela  Becchi 1.4.1"
#endif

int VERBOSE;
int DEBUG;
unsigned int all_sL = 0;
unsigned int cnt_for = 0;

/*
 * Returns the current version string
 */
void version(){
    printf("version:: %s\n", CUR_VER);
}

/* usage */
static void usage()
{
    fprintf(stderr,"\n");
    fprintf(stderr, "Usage: regex [options]\n");
    fprintf(stderr, "             [--parse|-p <regex_file> [--m|--i] | --import|-i <in_file> ]\n");
    fprintf(stderr, "             [--export|-e  <out_file>][--graph|-g <dot_file>]\n");
    fprintf(stderr, "             [--trace|-t <trace_file>]\n");
    fprintf(stderr, "             [--hfa]\n\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "    --help,-h       print this message\n");
    fprintf(stderr, "    --version,-r    print version number\n");
    fprintf(stderr, "    --verbose,-v    basic verbosity level \n");
    fprintf(stderr, "    --debug,  -d    enhanced verbosity level \n");
    fprintf(stderr, "\nOther:\n");
    fprintf(stderr, "    --parse,-p <regex_file>  process regex file\n");
    //fprintf(stderr, "    --method <0|1|2>         choose dfa construction method.\n");
    fprintf(stderr, "    --k k_value<defualt:2>   set k-reduction k.\n");
    fprintf(stderr, "\n");
    exit(0);
}

/* configuration */
static struct conf {
    char *regex_file;
    char *in_file;
    char *out_file;
    char *dot_file;
    char *trace_file;
    bool i_mod;
    bool m_mod;
    bool verbose;
    bool debug;
    bool hfa;
    int method;
    int k;
} config;

/* initialize the configuration */
void init_conf(){
    config.regex_file=NULL;
    config.in_file=NULL;
    config.out_file=NULL;
    config.dot_file=NULL;
    config.trace_file=NULL;
    config.i_mod=false;
    config.m_mod=false;
    config.debug=false;
    config.verbose=false;
    config.hfa=false;

    //
    config.method = 0;
    config.k = 2;
}

/* print the configuration */
void print_conf(){
    fprintf(stderr,"\nCONFIGURATION: \n");
    if (config.regex_file) fprintf(stderr, "- RegEx file: %s\n",config.regex_file);
    if (config.in_file) fprintf(stderr, "- DFA import file: %s\n",config.in_file);
    if (config.out_file) fprintf(stderr, "- DFA export file: %s\n",config.out_file);
    if (config.dot_file) fprintf(stderr, "- DOT file: %s\n",config.dot_file);
    if (config.trace_file) fprintf(stderr,"- Trace file: %s\n",config.trace_file);
    if (config.i_mod) fprintf(stderr,"- ignore case selected\n");
    if (config.m_mod) fprintf(stderr,"- m modifier selected\n");
    if (config.verbose && !config.debug) fprintf(stderr,"- verbose mode\n");
    if (config.debug) fprintf(stderr,"- debug mode\n");
    if (config.hfa)   fprintf(stderr,"- hfa generation invoked\n");
    if(config.method == 1) fprintf(stderr, "hierarchical mering construction.\n");
    if(config.method == 2) fprintf(stderr, "improved hierarchical mering construction.\n");
    if(config.method != 0) fprintf(stderr, "%d-reduction\n",config.k);
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
        }else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--version") == 0){
            version();
            return 0;
        }else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0){
            config.verbose=1;
        }else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0){
            config.debug=1;
        }else if(strcmp(argv[i], "--hfa") == 0){
            config.hfa=1;
        }else if(strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--graph") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Dot file name missing.\n");
                return 0;
            }
            config.dot_file=argv[i];
        }else if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--import") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Import file name missing.\n");
                return 0;
            }
            config.in_file=argv[i];
        }else if(strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--export") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Export file name missing.\n");
                return 0;
            }
            config.out_file=argv[i];
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
        }else if(strcmp(argv[i], "--method") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Method choice missing.\n");
                return 0;
            }
            config.method = atoi(argv[i]);
        }
        else if(strcmp(argv[i], "--k") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"k setting missing.\n");
                return 0;
            }
            config.k = atoi(argv[i]);
        }
        else{
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

DFA *dfas_merge(list<DFA*> *tomerge_lis, int k){
    //strong
    debugPrint("\nin dfas_merge...\n");
    if(k>KMAX) fatal("k is bigger than KMAX!");
    if(k==1) return tomerge_lis->front();
    int size = tomerge_lis->size();

    //init data
    DFA* dfa = new DFA();
    list<DfaStateLis> *mapping_queue = new list<DfaStateLis>();
    list<state_t > *queue = new list<state_t >;
    map<DfaStateLis, state_t > *mapProcessed_ptr = new map<DfaStateLis, state_t >();
    map<DfaStateLis, state_t > mapProcessed = *mapProcessed_ptr;
    list<DFA*>::iterator it_dfa;
    //list<DfaStateLis *> *trash_list = new list<DfaStateLis *>();

    //start node
    state_t startState = dfa->add_state();
    //DfaStateLis *startLis = new DfaStateLis(k);
    DfaStateLis startLis(k);
    for(int i=0; i<size; i++) startLis.set_value(i, 0);
    queue->push_back(startState);
    mapping_queue->push_back(startLis);
    mapProcessed[startLis] = startState;
    /*ACCEPTING*/
    FOREACH_DFA_LIST(tomerge_lis, it) dfa->accepts(startState)->add((*it)->accepts(0));

    debugPrint("\nbefore dfas_merge while.\n");
    while(!queue->empty()){
        state_t currentState = queue->front(); queue->pop_front();
        DfaStateLis currentLis = mapping_queue->front(); mapping_queue->pop_front();
        /*TODO : ACCEPT STATE*/
        int cnt_dfa=0;
        FOREACH_DFA_LIST(tomerge_lis, it) dfa->accepts(currentState)->add((*it)->accepts(currentLis[cnt_dfa++]));
        //debugPrint("currentLis:");
        //debugPrintLis(currentLis);
        //getchar();
        for(symbol_t c=0; c<CSIZE; c++){
            //DfaStateLis *targetLis = new DfaStateLis(k);
            DfaStateLis targetLis(k);
            //for(int i=0; i<size; i++)
            int cnt=0;
            FOREACH_DFA_LIST(tomerge_lis, it_dfa)
            {
                targetLis.set_value(cnt, (*it_dfa)->get_next_state(currentLis[cnt], c));
                cnt++;
            }
            map<DfaStateLis, state_t >::iterator it_map = mapProcessed.find(targetLis);
            state_t nextState;
            if(it_map != mapProcessed.end()){//found
                //debugPrint("\nfound!\n");
                nextState = it_map->second;
                //delete targetLis;
            }
            else{//not found
                //debugPrint("targetLis:");
                //debugPrintLis(*targetLis);
                //debugPrint("\nnot found!\n");
                nextState = dfa->add_state();
                queue->push_back(nextState);
                mapping_queue->push_back(targetLis);
                mapProcessed[targetLis] = nextState;
            }
            dfa->add_transition(currentState, c, nextState);
        }
    }

    debugPrint("\nfinished dfas_merge while.\n");
    //free space
    //free(queue);
    delete queue;
    delete mapping_queue;
    delete mapProcessed_ptr;
    //delete tomergelist 中的ptr， mapProcessed_ptr 中的ptr
    FOREACH_DFA_LIST(tomerge_lis, it) delete(*it);

    debugPrint("\nfinished dfas_merge.\n");
    return dfa;
}

DFA *dfas_merge_improve_dirty(list<DFA*> *tomerge_lis, int k){
    //strong
    debugPrint("\nin dfas_merge...\n");
    if(k>KMAX) fatal("k is bigger than KMAX!");
    if(k==1) return tomerge_lis->front();
    int size = tomerge_lis->size();

    //Do DFA minimization and Initialize Mask_i for Di
    FOREACH_DFA_LIST(tomerge_lis, it)
    {
        //fprintf(stderr, "\nbefore minimize dfa size:%d\n", (*it)->size());
        //(*it)->minimize();//what's wrong
        //fprintf(stderr, "\nafter minimize dfa size:%d\n", (*it)->size());
        //getchar();
        (*it)->initialize_masktable();
    }


    //init data
    DFA* dfa = new DFA(50, true);
    list<DfaStateLis> *mapping_queue = new list<DfaStateLis>();
    list<state_t > *queue = new list<state_t >;
    map<DfaStateLis, state_t > *mapProcessed_ptr = new map<DfaStateLis, state_t >();
    map<DfaStateLis, state_t > mapProcessed = *mapProcessed_ptr;
    list<DFA*>::iterator it_dfa;
    //list<DfaStateLis *> *trash_list = new list<DfaStateLis *>();

    //start node
    state_t startState = dfa->add_state();
    //DfaStateLis *startLis = new DfaStateLis(k);
    DfaStateLis startLis(k);
    for(int i=0; i<size; i++) startLis.set_value(i, 0);
    queue->push_back(startState);
    mapping_queue->push_back(startLis);
    mapProcessed[startLis] = startState;
    /*ACCEPTING*/
    FOREACH_DFA_LIST(tomerge_lis, it) dfa->accepts(startState)->add((*it)->accepts(0));

    debugPrint("\nbefore dfas_merge while.\n");
    while(!queue->empty()){
        //fprintf(stderr, "loc5");
        state_t currentState = queue->front(); queue->pop_front();
        DfaStateLis currentLis = mapping_queue->front(); mapping_queue->pop_front();
        /*TODO : ACCEPT STATE*/
        int cnt_dfa=0;
        //fprintf(stderr, "loc6");
        FOREACH_DFA_LIST(tomerge_lis, it)
        {
            /*fprintf(stderr, "loc7");
            if(dfa->accepts(currentState) == NULL) fprintf(stderr, "\n1 null\n");
            fprintf(stderr, "loc8");
            if((*it)->accepts(cnt_dfa) == NULL) fprintf(stderr, "\n2 null\n");
            fprintf(stderr, "loc9");*/
            dfa->accepts(currentState)->add((*it)->accepts(currentLis[cnt_dfa++]));
        }
        //fprintf(stderr, "loc7");
        //debugPrint("currentLis:");
        //debugPrintLis(currentLis);
        //getchar();
        for(symbol_t c=0; c<CSIZE; ){
            //fprintf(stderr, "in for, c:%d",c);
            //DfaStateLis *targetLis = new DfaStateLis(k);
            DfaStateLis targetLis(k);
            //for(int i=0; i<size; i++)
            int cnt=0;
            unsigned char segmentLen=255;
            //fprintf(stderr, "loc0");
            FOREACH_DFA_LIST(tomerge_lis, it_dfa)
            {
                state_t s = currentLis[cnt];
                targetLis.set_value(cnt, (*it_dfa)->get_next_state(s, c));
                //fprintf(stderr, "loc0.1");
                //fprintf(stderr,"\nsegmentLen=%d\n",segmentLen);
                segmentLen = min(segmentLen, (*it_dfa)->mask_table[s][c]);
                //fprintf(stderr, "loc0.2");
                cnt++;
            }
            //fprintf(stderr, "loc0.1");
            cnt=0;
            FOREACH_DFA_LIST(tomerge_lis, it_dfa){
                int tem_cnt = cnt;
                cnt++;
                if((*it_dfa)->mask_table[currentLis[tem_cnt]][c] == segmentLen) continue;
                unsigned char nc = c+segmentLen+1;
                //if((*it_dfa)->mask_table[currentLis[tem_cnt]][nc] == (*it_dfa)->mask_table[currentLis[tem_cnt]][c] - segmentLen-1) continue;
                (*it_dfa)->mask_table[currentLis[tem_cnt]][nc] = (*it_dfa)->mask_table[currentLis[tem_cnt]][c] - segmentLen-1;
                (*it_dfa)->add_transition(currentLis[tem_cnt], nc, targetLis[tem_cnt]);

            }
            //fprintf(stderr, "loc1");

            map<DfaStateLis, state_t >::iterator it_map = mapProcessed.find(targetLis);
            state_t nextState;
            if(it_map != mapProcessed.end()){//found
                nextState = it_map->second;
            }
            else{//not found
                nextState = dfa->add_state();
                queue->push_back(nextState);
                mapping_queue->push_back(targetLis);
                mapProcessed[targetLis] = nextState;
            }
            //fprintf(stderr, "loc2");

            dfa->add_transition(currentState,c,nextState);
            dfa->mask_table[currentState][c] = segmentLen;
            c += (segmentLen+1);
            /*
            state_t upper = c+segmentLen+1;
            for(;c<upper;c++)
            {
                //fprintf(stderr,"\nin inner for,c=%d\n",c);
                dfa->add_transition(currentState, c, nextState);
            }*/
            //fprintf(stderr, "loc3");
        }
        //fprintf(stderr, "loc4");
    }

    debugPrint("\nfinished dfas_merge while.\n");
    //free space
    //free(queue);
    delete queue;
    delete mapping_queue;
    delete mapProcessed_ptr;
    //delete tomergelist 中的ptr， mapProcessed_ptr 中的ptr
    FOREACH_DFA_LIST(tomerge_lis, it) delete(*it);

    debugPrint("\nfinished dfas_merge.\n");
    return dfa;
}


DFA *dfas_merge_improve(list<DFA*> *tomerge_lis, int k){
    //strong
    debugPrint("\nin dfas_merge...\n");
    if(k>KMAX) fatal("k is bigger than KMAX!");
    if(k==1) return tomerge_lis->front();
    int size = tomerge_lis->size();

    //Do DFA minimization and Initialize Mask_i for Di
    FOREACH_DFA_LIST(tomerge_lis, it)
    {
        //fprintf(stderr, "\nbefore minimize dfa size:%d\n", (*it)->size());
        //(*it)->minimize();//what's wrong
        //fprintf(stderr, "\nafter minimize dfa size:%d\n", (*it)->size());
        //getchar();
        (*it)->initialize_masktable();
    }


    //init data
    DFA* dfa = new DFA();
    list<DfaStateLis> *mapping_queue = new list<DfaStateLis>();
    list<state_t > *queue = new list<state_t >;
    map<DfaStateLis, state_t > *mapProcessed_ptr = new map<DfaStateLis, state_t >();
    map<DfaStateLis, state_t > mapProcessed = *mapProcessed_ptr;
    list<DFA*>::iterator it_dfa;
    //list<DfaStateLis *> *trash_list = new list<DfaStateLis *>();

    //start node
    state_t startState = dfa->add_state();
    //DfaStateLis *startLis = new DfaStateLis(k);
    DfaStateLis startLis(k);
    for(int i=0; i<size; i++) startLis.set_value(i, 0);
    queue->push_back(startState);
    mapping_queue->push_back(startLis);
    mapProcessed[startLis] = startState;
    /*ACCEPTING*/
    FOREACH_DFA_LIST(tomerge_lis, it) dfa->accepts(startState)->add((*it)->accepts(0));

    debugPrint("\nbefore dfas_merge while.\n");
    while(!queue->empty()){
        //fprintf(stderr, "loc5");
        state_t currentState = queue->front(); queue->pop_front();
        DfaStateLis currentLis = mapping_queue->front(); mapping_queue->pop_front();
        /*TODO : ACCEPT STATE*/
        int cnt_dfa=0;
        //fprintf(stderr, "loc6");
        FOREACH_DFA_LIST(tomerge_lis, it) dfa->accepts(currentState)->add((*it)->accepts(currentLis[cnt_dfa++]));
        //fprintf(stderr, "loc7");
        //debugPrint("currentLis:");
        //debugPrintLis(currentLis);
        //getchar();
        for(symbol_t c=0; c<CSIZE; ){
            //fprintf(stderr, "in for, c:%d",c);
            //DfaStateLis *targetLis = new DfaStateLis(k);
            DfaStateLis targetLis(k);
            //for(int i=0; i<size; i++)
            int cnt=0;
            unsigned char segmentLen=255;
            //fprintf(stderr, "loc0");
            FOREACH_DFA_LIST(tomerge_lis, it_dfa)
            {
                state_t s = currentLis[cnt];
                targetLis.set_value(cnt, (*it_dfa)->get_next_state(s, c));
                //fprintf(stderr, "loc0.1");
                //fprintf(stderr,"\nsegmentLen=%d\n",segmentLen);
                segmentLen = min(segmentLen, (*it_dfa)->mask_table[s][c]);
                //fprintf(stderr, "loc0.2");
                cnt++;
            }

            //fprintf(stderr, "loc1");

            map<DfaStateLis, state_t >::iterator it_map = mapProcessed.find(targetLis);
            state_t nextState;
            if(it_map != mapProcessed.end()){//found
                nextState = it_map->second;
            }
            else{//not found
                nextState = dfa->add_state();
                queue->push_back(nextState);
                mapping_queue->push_back(targetLis);
                mapProcessed[targetLis] = nextState;
            }
            //fprintf(stderr, "loc2");


            state_t upper = c+segmentLen+1;
            for(;c<upper;c++)
            {
                //fprintf(stderr,"\nin inner for,c=%d\n",c);
                dfa->add_transition(currentState, c, nextState);
            }

            cnt_for+=1;
            all_sL += segmentLen;
        }
    }

    debugPrint("\nfinished dfas_merge while.\n");
    //free space
    delete queue;
    delete mapping_queue;
    delete mapProcessed_ptr;
    //delete tomergelist 中的ptr， mapProcessed_ptr 中的ptr
    FOREACH_DFA_LIST(tomerge_lis, it) delete(*it);

    debugPrint("\nfinished dfas_merge.\n");
    return dfa;
}

DFA *hm_nfa2dfa(list<NFA*> *nfa_list, int size, int k=2,bool improved=false)
{
    debugPrint("\nin hm_nfa2dfa...\n");
    //step1. single nfa2dfa
    time_t start_t, end_t;
    time(&start_t);
    list<DFA*> *dfa_list = new list<DFA*>();
    list<NFA*>::iterator it_nfa;

#pragma omp parallel for default(shared)
    for(int i=0; i<size; i++)
    {
        fprintf(stderr, "single nfa2dfa %d/%d\n", i+1, size);
        NFA *nfa;
#pragma omp critical (section1)
        {
            nfa = nfa_list->front();
            nfa_list->pop_front();
        }
        //apply M.becchi reduce & remove_epsilon
        nfa->remove_epsilon();
        //fprintf(stderr, "nfa size: %d\n", nfa->size());
        //nfa->reduce();
        //fprintf(stderr, "nfa size: %d\n", nfa->size());
        DFA* dfa = nfa->nfa2dfa();
        //fprintf(stderr, "dfa size: %d\n", dfa->size());
        //DFA* dfa = nfa->smallSize_nfa2dfa();
#pragma omp critical (section2)
        {
            dfa_list->push_back(dfa);
        }
        //fprintf(stderr, "single dfa size:%d\n", dfa->size());
        delete nfa;
    }
    time(&end_t);
    fprintf(stderr, "step1 single nfa2dfa time: %f seconds.\n", difftime(end_t, start_t));
    //step2 hierarchical merging
    unsigned long dfa_amount = size;
    while(dfa_amount != 1){
        fprintf(stderr,"dfa_amout:%d\n", dfa_amount);
        list<DFA*> *next_round_dfa_lis = new list<DFA*>();
#pragma omp parallel for default(shared)
        for(int i=0; i<((dfa_amount-1)/k + 1); i++)
        {
            list<DFA *> *dfa_tomerge_lis = new list<DFA*>();
            for(int j=0; j<k; j++) {//get front k dfa
                DFA *tem_dfa;
                int flag = false;
#pragma omp critical (section3)
                {
                    if(dfa_list->empty()) flag=true;
                    if(!flag) {
                        tem_dfa = dfa_list->front();
                        dfa_list->pop_front();
                    }
                }
                if(flag) break;
                dfa_tomerge_lis->push_front(tem_dfa);
            }
            DFA* target_dfa;
            if(improved) target_dfa = dfas_merge_improve(dfa_tomerge_lis, dfa_tomerge_lis->size());
            else target_dfa = dfas_merge(dfa_tomerge_lis, dfa_tomerge_lis->size());
#pragma omp critical (section4)
            {
                next_round_dfa_lis->push_front(target_dfa);
            }
            //free
            delete dfa_tomerge_lis;
        }
        delete dfa_list;
        dfa_list = next_round_dfa_lis;
        dfa_amount = dfa_list->size();
    }

    //assert(dfa_list->size() == 1);
    //free
    DFA* finalDFA = dfa_list->front();
    dfa_list->pop_front();
    delete dfa_list;

    return finalDFA;
}

/*
 *  MAIN - entry point
 */
int main(int argc, char **argv){

    //read configuration
    init_conf();

    while(!parse_arguments(argc,argv)) usage();
    print_conf();
    VERBOSE=config.verbose;
    DEBUG=config.debug; if (DEBUG) VERBOSE=1;

    //check that it is possible to open the files
    if (config.regex_file!=NULL) check_file(config.regex_file,"r");
    if (config.in_file!=NULL) check_file(config.in_file,"r");
    if (config.out_file!=NULL) check_file(config.out_file,"w");
    if (config.dot_file!=NULL) check_file(config.dot_file,"w");
    if (config.trace_file!=NULL) check_file(config.trace_file,"r");

    // check that either a regex file or a DFA import file are given as input
    if (config.regex_file==NULL && config.in_file==NULL){
        fatal("No data file - please use either a regex or a DFA import file\n");
    }
    if (config.regex_file!=NULL && config.in_file!=NULL){
        printf("DFA will be imported from the Regex file. Import file will be ignored");
    }

    /* FA declaration */
    NFA *nfa=NULL;  	// NFA
    DFA *dfa=NULL;		// DFA


    //improved hierachical merging method
    if (config.regex_file!=NULL){
        FILE *regex_file=fopen(config.regex_file,"r");
        fprintf(stderr,"\nParsing the regular expression file %s ...\n",config.regex_file);
        regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);
        int size;
        list<NFA *> *nfa_list;
        nfa_list = parse->parse_to_list(regex_file, &size);

        time_t start_t, end_t;
        time(&start_t);
        dfa=hm_nfa2dfa(nfa_list, size, config.k, true);
        time(&end_t);
        fprintf(stderr, "hm_improve dfa construction time: %llf seconds.\n", difftime(end_t, start_t));

        if (dfa==NULL) printf("Max DFA size %ld exceeded during creation: the DFA was not generated\n",MAX_DFA_SIZE);
        fclose(regex_file);
        delete parse;
        delete(nfa_list);

        //verification
        fprintf(stderr, "hm_improved gen dfa size:%d\n", dfa->size());
    }
    return 0;

}
