//
// Created by 钟金诚 on 2020/5/16.
//
#include <string>
#include <sys/time.h>
#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "ecdfa.h"
#include "hybrid_fa.h"
#include "parser.h"
#include "trace.h"
#include "hierarMerging.h"

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

DFA *dfas_merge_improve(list<DFA*> *tomerge_lis, int k){
    //strong
    if(k>KMAX) fatal("k is bigger than KMAX!");
    if(k==1) return tomerge_lis->front();
    int size = tomerge_lis->size();

    //Do DFA minimization and Initialize Mask_i for Di
    FOREACH_DFA_LIST(tomerge_lis, it)
    {
        //fprintf(stderr, "\nbefore minimize dfa size:%d\n", (*it)->size());
        //(*it)->minimize();//what's wrong
        //fprintf(stderr, "\nafter minimize dfa size:%d\n", (*it)->size());
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

    while(!queue->empty()){
        //fprintf(stderr, "loc5");
        state_t currentState = queue->front(); queue->pop_front();
        DfaStateLis currentLis = mapping_queue->front(); mapping_queue->pop_front();
        /*TODO : ACCEPT STATE*/
        int cnt_dfa=0;
        FOREACH_DFA_LIST(tomerge_lis, it) dfa->accepts(currentState)->add((*it)->accepts(currentLis[cnt_dfa++]));
        for(symbol_t c=0; c<CSIZE; ){
            DfaStateLis targetLis(k);
            int cnt=0;
            unsigned char segmentLen=255;
            FOREACH_DFA_LIST(tomerge_lis, it_dfa)
            {
                state_t s = currentLis[cnt];
                targetLis.set_value(cnt, (*it_dfa)->get_next_state(s, c));
                segmentLen = min(segmentLen, (*it_dfa)->mask_table[s][c]);
                cnt++;
            }

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

                /*border for hfa*/
                nfa_set *border_nfaset = new nfa_set ();
                int ind = 0;
                FOREACH_DFA_LIST(tomerge_lis, it){
                    assert(ind < targetLis.size);
                    state_t dfastate = targetLis.value[ind++];
                    map<state_t, nfa_set*> *border_tmp = (map<state_t, nfa_set*>*) (*it)->border;
                    map <state_t,nfa_set*>::iterator map_it = border_tmp->find(dfastate);
                    if(map_it != border_tmp->end()){
                        border_nfaset->insert(map_it->second->begin(), map_it->second->end());
                    }
                }
                if(border_nfaset->size() != 0){
                    map<state_t, nfa_set*> *border = (map<state_t, nfa_set*>*) dfa->border;
                    (*border)[nextState] = border_nfaset;
                }
                else{
                    delete border_nfaset;
                }
            }

            state_t upper = c+segmentLen+1;
            for(;c<upper;c++)
            {
                //fprintf(stderr,"\nin inner for,c=%d\n",c);
                dfa->add_transition(currentState, c, nextState);
            }
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

DFA *hm_dfalist2dfa(list<DFA*> *dfa_list, int k, bool improved){
    assert(improved); // only implement improved version
    unsigned long dfa_amount = dfa_list->size();
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
            //assert(improved);
            target_dfa = dfas_merge_improve(dfa_tomerge_lis, dfa_tomerge_lis->size());
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

DFA *hm_nfa2dfa(list<NFA*> *nfa_list, int k, bool improved)
{
    debugPrint("\nin hm_nfa2dfa...\n");

    int size = nfa_list->size();
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