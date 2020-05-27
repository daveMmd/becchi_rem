//
// Created by 钟金诚 on 2019/8/27.
//

#include "nfa.h"
#include "dave_util.h"

void NFA::build_bitset(){
    //init
    for(int c=0; c<CSIZE; c++) succs.push_back(new bitset<MAX_NFA_SIZE>());
    //set
    for(int c=0; c<CSIZE; c++)
    {
        nfa_set *succ_set = new nfa_set();
        //get succ_states set
        nfa_set *succ_c = this->get_transitions(c);
        if(succ_c == NULL) continue;
        FOREACH_SET(succ_c,it){
            nfa_set *epsilon_set = (*it)->epsilon_closure();
            succ_set->insert(epsilon_set->begin(), epsilon_set->end());
            delete(epsilon_set);
        }

        //succ_states to bitset
        FOREACH_SET(succ_set, it){
            //succs[c][(*it)->id]=1;
            succs[c]->set((*it)->id);
        }
        delete succ_c;
        delete(succ_set);
    }
    //to verify

}
void NFA::init_bitset() {
    nfa_set *allStates = new nfa_set();
    this->traverse(allStates);

    FOREACH_SET(allStates, it) {
        (*it)->build_bitset();
    }
    delete allStates;
}

bitset<MAX_NFA_SIZE> * states2bits(nfa_set* nfa_states)
{
    bitset<MAX_NFA_SIZE> *bits = new bitset<MAX_NFA_SIZE>();
    FOREACH_SET(nfa_states, it){
        bits->set((*it)->id);
    }
    return bits;
}

nfa_set* NFA::bits2states(bitset<MAX_NFA_SIZE> &bits){
    nfa_set* nfa_states = new nfa_set();
    for(int i=0; i<_size; i++){
        if(bits[i]) nfa_states->insert(id2state[i]);
    }
    return nfa_states;
}

void NFA::get_id2state() {
    for(int i=0; i<_size; i++) id2state.push_back(NULL);
    nfa_set *allStates = new nfa_set();
    this->traverse(allStates);
    if(allStates == NULL) debugPrint("allStates NULL!");
    FOREACH_SET(allStates, it) {
        id2state[(*it)->id] = *it;
    }
    delete allStates;
}

struct MyCompare{
    bool operator()(const bitset<MAX_NFA_SIZE> &b1, const bitset<MAX_NFA_SIZE> &b2) const{
        //return (p1.age < p2.age) || (p1.age == p2.age && p1.name.length() < p2.name.length());
        for(int i=0; i<MAX_NFA_SIZE; i++){
            if(!b1.test(i) && (b2.test(i))) return true;
            if(b1.test(i) && (!b2.test(i))) return false;
        }
        return false;
    }
};

DFA *NFA::smallSize_nfa2dfa(){
    // DFA to be created
    //debugPrint("\nin smallSize_nfa2dfa...\n");
    if(this->size() > MAX_NFA_SIZE) fatal("NFA SIZE bigger than MAX_NFA_SIZE");
    //add
    this->init_bitset();
    this->get_id2state();
    //getchar();//later is not finished.

    //data structure to use
    DFA *dfa=new DFA();
    // contains mapping between DFA and NFA set of states
    map<bitset<MAX_NFA_SIZE>, state_t, MyCompare> *mapping_ptr = new map<bitset<MAX_NFA_SIZE>, state_t, MyCompare>();
    map<bitset<MAX_NFA_SIZE>, state_t, MyCompare> mapping = *mapping_ptr;
    //queue of DFA states to be processed and of the set of NFA states they correspond to
    list <state_t> *queue = new list<state_t>();
    list <nfa_set*> *mapping_queue = new list<nfa_set*>();
    nfa_set::iterator set_it;
    bitset<MAX_NFA_SIZE> succMask;
    map<bitset<MAX_NFA_SIZE>, state_t>::iterator map_it;
    state_t succState;

    //start node
    state_t startState=dfa->add_state();
    queue->push_back(startState);
    nfa_set* nfa_states = this->epsilon_closure();
    mapping_queue->push_back(nfa_states);
    bitset<MAX_NFA_SIZE>* startBits = states2bits(nfa_states);
    mapping[*startBits] = startState;
    delete(startBits);
    /*TODO: ACCEPT*/


    //while loop
    while(!queue->empty()){
        //dequeue an element
        state_t state=queue->front(); queue->pop_front();
        nfa_set *cl_states=mapping_queue->front(); mapping_queue->pop_front();
        for(int c=0; c<CSIZE; c++){
            //get succMask
            succMask.reset();
            FOREACH_SET(cl_states, set_it){
                succMask |= *((*set_it)->succs[c]);
            }
            //check if processed
            map_it = mapping.find(succMask);
            if(map_it == mapping.end()){//not found
                nfa_set *succStates = bits2states(succMask);
                succState = dfa->add_state();
                queue->push_back(succState);
                mapping_queue->push_back(succStates);
                mapping[succMask] = succState;
            }
            else{
                succState = (*map_it).second;
            }
            dfa->add_transition(state, c, succState);
        }
        delete(cl_states);
    }


    delete queue;
    delete mapping_queue;
    delete mapping_ptr;
    return dfa;
}
