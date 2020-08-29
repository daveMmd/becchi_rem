//
// Created by 钟金诚 on 2020/7/22.
//

#include "Fb_NFA.h"
#include "subset.h"
#include "dave_subset.h"

Fb_NFA::Fb_NFA(int size) {
    _size = size;
    stt = (set<state_t> ***) malloc(sizeof(set<state_t> **) * (_size + 5));
    for(state_t s=0; s<_size; s++){
        stt[s] = (set<state_t> **) malloc(sizeof(set<state_t> *) * 16);
        for(int c=0; c<16; c++) stt[s][c] = new set<state_t>();
    }
    start_states.clear();
}

Fb_NFA::~Fb_NFA() {
    for(int s = 0; s < _size; s++){
        for(int c = 0; c < 16; c++) delete stt[s][c];
        delete stt[s];
    }
    delete stt;
}

typedef set<state_t> Fb_nfaset;
//powerset construction & subset construction
Fb_DFA * Fb_NFA::nfa2dfa() {
    Fb_DFA* dfa = new Fb_DFA();

    // contains mapping between DFA and NFA set of states
#if IMPROVE_SUBSET
    dave_subset *mapping = new dave_subset();
#else
    subset *mapping=new subset(0);
#endif
    //queue of DFA states to be processed and of the set of NFA states they correspond to
    list<state_t> *queue = new list<state_t>();
    list<Fb_nfaset *> *mapping_queue = new list<Fb_nfaset *>();
    //iterators used later on
    Fb_nfaset::iterator set_it;
    //new dfa state id
    state_t target_state=NO_STATE;
    //set of nfas state corresponding to target dfa state
    Fb_nfaset *target=new Fb_nfaset(); //in FA form

    /* code begins here */
    //initialize data structure starting from INITIAL STATE
    for(int s=0; s<_size; s++)
        if(start_states.find(s) != start_states.end()) target->insert(s);

    Fb_nfaset *temp = new Fb_nfaset();
    temp->insert(target->begin(), target->end());
    mapping->lookup(temp, dfa, &target_state);
#if !IMPROVE_SUBSET
    delete temp;
#endif

    //FOREACH_SET(target,set_it)  dfa->accepts(target_state)->add((*set_it)->accepting);
    for(set_it = target->begin(); set_it != target->end(); set_it++)
        if(accept_states.find(*set_it) != accept_states.end())
            dfa->is_accept[target_state] = 1;
    queue->push_back(target_state);
    mapping_queue->push_back(target);

    // process the states in the queue and adds the not yet processed DFA states
    // to it while creating them
#define MAX_STATES_NUMBER_ 100005
    int mark[MAX_STATES_NUMBER_];
    memset(mark, 0, sizeof(int) * MAX_STATES_NUMBER_);
    while (!queue->empty()){
        //dequeue an element
        state_t state=queue->front(); queue->pop_front();
        Fb_nfaset *cl_state=mapping_queue->front(); mapping_queue->pop_front();
        // each state must be processed only once
        if(!mark[state]){
            mark[state] = 1;
            //iterate other all characters and compute the next state for each of them
            for(symbol_t i=0;i<16;i++){
                target= new Fb_nfaset();
                for(set_it = cl_state->begin(); set_it != cl_state->end(); set_it++){
                    Fb_nfaset *state_set = stt[*set_it][i];
                    target->insert(state_set->begin(), state_set->end());
                }
                //look whether the target set of state already corresponds to a state in the DFA
                //if the target set of states does not already correspond to a state in a DFA,
                //then add it
                Fb_nfaset *temp = new Fb_nfaset();
                temp->insert(target->begin(), target->end());
                bool found=mapping->lookup(temp, dfa, &target_state);
#if !IMPROVE_SUBSET
                delete temp;
#endif

                if (target_state == MAX_DFA_SIZE*5){
                    delete queue;
                    while (!mapping_queue->empty()){
                        delete mapping_queue->front();
                        mapping_queue->pop_front();
                    }
                    delete mapping_queue;
                    delete mapping;
                    delete dfa;
                    delete target;
                    delete cl_state;
                    return NULL;
                }
                if (!found){
                    queue->push_back(target_state);
                    mapping_queue->push_back(target);

                    for(set_it = target->begin(); set_it != target->end(); set_it++)
                        if(accept_states.find(*set_it) != accept_states.end())
                            dfa->is_accept[target_state] = 1;
                    /*FOREACH_SET(target,set_it){
                        dfa->accepts(target_state)->add((*set_it)->accepting);
                    }*/
                    //if (target->empty()) dfa->set_dead_state(target_state);
                }else{
                    delete target;
                }
                dfa->add_transition(state, i, target_state); // add transition to the DFA
            }//end for on character i
        }//end if state marked
        delete cl_state;
    }//end while
    //deallocate all the sets and the state_mapping data structure
    delete queue;
    delete mapping_queue;
    //if (DEBUG) mapping->dump(); //dumping the NFA-DFA number of state information
    delete mapping;

    return dfa;
}

void Fb_NFA::to_dot(char *fname, char *title) {
    FILE* file = fopen(fname, "w");
    fprintf(file, "digraph \"%s\" {\n", title);

    for (state_t s=0;s<_size;s++){
        if(s%17)
            fprintf(file, " %ld [shape=circle,label=\"%ld-%ld\"];\n", s, s/17, s%17);
        else
            fprintf(file, " %ld [shape=doublecircle,label=\"%ld-%ld\"];\n", s, s/17, s%17);
    }
    int csize = 16;
    int *mark=allocate_int_array(csize);
    char *label=NULL;
    char *temp=(char *)malloc(100);
    state_t target=NO_STATE;
    for (state_t s=0;s<_size;s++){
        if(s == 0) continue;
        for(int i=0;i<csize;i++) mark[i]=0;
        for (int c=0;c<csize;c++){
            if (!mark[c]){
                mark[c]=1;
                if (!stt[s][c]->empty()){
                    set<state_t>::iterator iter;
                    for(iter = stt[s][c]->begin(); iter != stt[s][c]->end(); iter++){
                        sprintf(temp, "\\x%02d", c);
                        fprintf(file, "%ld -> %ld [label=\"%s\"];\n", s, *iter, temp);
                    }
                }
            }
            if (label!=NULL) {
                fprintf(file, "%ld -> %ld [label=\"%s\"];\n", s,target,label);
                free(label);
                label=NULL;
            }
        }
        //if (default_tx!=NULL) fprintf(file, "%ld -> %ld [color=\"limegreen\"];\n", s,default_tx[s]);
    }
    free(temp);
    free(mark);
    fprintf(file,"}");
    fclose(file);
}