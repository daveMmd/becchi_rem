//
// Created by 钟金诚 on 2020/8/19.
//

#ifndef BECCHI_REGEX_DAVE_SUBSET_H
#define BECCHI_REGEX_DAVE_SUBSET_H
#define IMPROVE_SUBSET 1
#include "stdinc.h"
#include "dfa.h"
#include "nfa.h"
#include "Fb_DFA.h"
#include <set>
#include <unordered_map>

using namespace std;

struct myhasher
{
public:
    size_t operator()( set<state_t> * const& s) const{
        size_t hash_value = 0;
        for(auto &it: *s){
            hash_value = hash_value ^ hash<state_t>()(it);
        }
        return hash_value;
    }
};

struct myequaler{
public:
    bool operator()(set<state_t> * const& s1, set<state_t> * const& s2) const{
        if(s1->size() != s2->size()) return false;
        auto it1 = s1->begin();
        auto it2 = s2->begin();
        while(it1 != s1->end()){
            if(*it1 != *it2) return false;
            it1++; it2++;
        }
        return true;
    }
};

class dave_subset{
    //unordered_map<set<state_t>>
    unordered_map<set<state_t> *, state_t, myhasher, myequaler> mapping;
public:
    //instantiates a new subset with the given state id
    //dave_subset();

    //(recursively) deallocates the subset
    ~dave_subset();

    // For subset construction:
    // - queries a subset
    // - if the subset is not present, then:
    //	 (1) creates a new state in the DFA
    //   (2) creates a new subset (updating the whole data structure), and associates it to the newly created DFA state
    // nfaid_set: set of nfa_id to be queried
    // dfa: dfa to be updated, in case a new state has to be created
    // dfaid: pointer to the variable containing the value of the (new) dfa_id
    // returns true if the subset (for nfaid_sets) was preesisting, and false if not. In the latter
    // case, a new DFA state is created
    bool lookup(set <state_t> *nfaid_set, DFA *dfa, state_t *dfa_id);
    bool lookup(set <state_t> *nfaid_set, Fb_DFA *dfa, state_t *dfa_id);
};
#endif //BECCHI_REGEX_DAVE_SUBSET_H
