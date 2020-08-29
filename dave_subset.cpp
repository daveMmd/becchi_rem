//
// Created by 钟金诚 on 2020/8/19.
//

#include "dave_subset.h"

bool dave_subset::lookup(set<state_t> *nfaid_set, DFA *dfa, state_t *dfa_id) {
    //unordered_map<set<state_t> *, state_t>::iterator it = mapping.find(nfaid_set);
    auto it = mapping.find(nfaid_set);

    if(it != mapping.end()){//find the key
        *dfa_id = it->second;
        return true;
    }
    else{
        state_t new_dfa_id = dfa->add_state();
        mapping[nfaid_set] = new_dfa_id;
        *dfa_id = new_dfa_id;
        return false;
    }
}

bool dave_subset::lookup(set<state_t> *nfaid_set, Fb_DFA *dfa, state_t *dfa_id) {
    auto it = mapping.find(nfaid_set);

    if(it != mapping.end()){//find the key
        *dfa_id = it->second;
        return true;
    }
    else{
        state_t new_dfa_id = dfa->add_state();
        mapping[nfaid_set] = new_dfa_id;
        *dfa_id = new_dfa_id;
        return false;
    }
}

dave_subset::~dave_subset() {
    for(auto &it: mapping){
        if(it.first != NULL) delete it.first;
    }
}