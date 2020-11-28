//
// Created by 钟金诚 on 2020/11/25.
//

#include "prefix_DFA.h"

bool prefix_DFA::pre_match(char* data, uint16_t offset){
    if(this->parent_node == nullptr) return true;
    if(offset+1 < depth) return true;

    offset -= depth; //减去prefix匹配长度

    list<prefix_DFA*> active_prefixDfas;
    list<state_t> active_states;
    active_prefixDfas.push_back(this->parent_node);
    active_states.push_back(0);

    while(1){
        char c = data[offset];

        list<prefix_DFA*> add_post_dfas;
        int add_cnt = 0;

        auto it_prefixdfa = active_prefixDfas.begin();
        for(auto it_state = active_states.begin(); it_state != active_states.end(); it_state++){

            DFA* iter_dfa = (*it_prefixdfa)->prefix_dfa;
            *it_state = iter_dfa->get_next_state(*it_state, (unsigned char)c);
            if(!iter_dfa->accepts(*it_state)->empty()){
                //only match once. here match occurs
                if((*it_prefixdfa)->next_node == nullptr)
                {
                    return true;
                }
                //activate post dfa (next char)
                prefix_DFA* candidate = (*it_prefixdfa)->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
            //clear dead state & dead prefixdfa
            if(*it_state == iter_dfa->dead_state){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
            //increment dfa iterator
            it_prefixdfa++;
        }

        active_prefixDfas.insert(active_prefixDfas.end(), add_post_dfas.begin(), add_post_dfas.end());
        active_states.insert(active_states.end(), add_cnt, 0);

        if(active_prefixDfas.empty()) break;


        if(offset == 0) break;
        offset -= 1;
    }

    return false;
}

void prefix_DFA::match(packet_t* packet, uint16_t offset, void (*callback)(packet_t *, char*)) {
    if(!pre_match(packet->data, offset)) return;

    list<prefix_DFA*> active_prefixDfas;
    list<state_t> active_states;

    if(this->next_node == nullptr) goto MATCH_SUCC;

    active_prefixDfas.push_back(this->next_node);
    active_states.push_back(0);

    while(++offset < packet->length){
        char c = packet->data[offset];

        list<prefix_DFA*> add_post_dfas;
        int add_cnt = 0;

        auto it_prefixdfa = active_prefixDfas.begin();
        for(auto it_state = active_states.begin(); it_state != active_states.end(); it_state++){

            DFA* iter_dfa = (*it_prefixdfa)->prefix_dfa;
            *it_state = iter_dfa->get_next_state(*it_state, (unsigned char)c);
            if(!iter_dfa->accepts(*it_state)->empty()){
                //only match once. here match occurs
                if((*it_prefixdfa)->next_node == nullptr)
                {
                    goto MATCH_SUCC;
                }
                //activate post dfa (next char)
                prefix_DFA* candidate = (*it_prefixdfa)->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
            //clear dead state & dead prefixdfa
            if(*it_state == iter_dfa->dead_state){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
            //increment dfa iterator
            it_prefixdfa++;
        }

        active_prefixDfas.insert(active_prefixDfas.end(), add_post_dfas.begin(), add_post_dfas.end());
        active_states.insert(active_states.end(), add_cnt, 0);

        if(active_prefixDfas.empty()) break;
    }

    return ;

MATCH_SUCC:
    if(callback != nullptr) callback(packet, this->complete_re);
}