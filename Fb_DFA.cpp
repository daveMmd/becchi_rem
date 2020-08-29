//
// Created by 钟金诚 on 2020/7/22.
//

#include "Fb_DFA.h"
#include "Fb_NFA.h"

/* 1.reverse accept states and start states
 * 2.reverse edges
 * */
Fb_NFA* reverse2nfa(Fb_DFA* dfa){
    Fb_NFA* nfa = new Fb_NFA(dfa->_size);
    //1.reverse accept states and start states
    for(state_t s=0; s<dfa->_size; s++){
        if(dfa->is_accept[s]) nfa->start_states.insert(s);
    }
    nfa->accept_states.insert(0);

    //2.reverse edges
    for(state_t s=0; s<dfa->_size; s++){
        for(int c=0; c<16; c++){
            state_t d = dfa->state_table[s][c];
            nfa->stt[d][c]->insert(s);
        }
    }

    return nfa;
}

Fb_DFA::Fb_DFA(DFA *dfa) {
    //init
    cons2state_num = -1;
    _size = 17 * dfa->size();// each node plus 16 new nodes --> 17 nodes
    state_table = (state_t **) malloc(sizeof(state_t *) * (_size + 5));
    for(int i = 0; i < _size + 5; i++){
        state_table[i] = (state_t *) malloc(sizeof(state_t) * 16);
    }
    is_accept = (int *) malloc(sizeof(int) * _size);
    memset(is_accept, 0, sizeof(int) * _size);

    //accept states
    for(state_t s=0; s<dfa->size(); s++){
        if(!dfa->accepts(s)->empty()){
            is_accept[s*17] = 1;
        }
    }

    /*build stt for fb_dfa*/
    state_t **stt_dfa = dfa->get_state_table();
    //add one node for each pre 4-bit
    for(state_t s=0; s<dfa->size(); s++){
        //pre 4-bit state trans
        for(int c=0; c<16; c++){
            state_table[s*17][c] = s * 17 + (c+1);
        }
        //post 4-bit state trans
        for(int c=0; c<256; c++){
            state_t news = s*17 + c/16 + 1;
            state_table[news][c%16] = stt_dfa[s][c] * 17;
        }
    }
}

#define MAX_DFA_SIZE_INCREMENT 50

state_t Fb_DFA::add_state() {
    state_t state=_size++;
    if(_size % 100000==0)fprintf(stderr, "dfa now states:%d\n",_size);
    if (state >= entry_allocated){
        //if (state >= entry_allocated-5){
        entry_allocated += MAX_DFA_SIZE_INCREMENT;
        state_table = reallocate_state_matrix(state_table, entry_allocated);
        for (state_t s=_size;s<entry_allocated;s++){
            state_table[s]=NULL;
        }

        is_accept = reallocate_int_array(is_accept, entry_allocated);
    }
    state_table[state] = allocate_state_array(16);
    int i; for (i=0;i<16;i++) state_table[state][i]=NO_STATE;

    return state;
}

Fb_DFA::Fb_DFA() {
    cons2state_num = -1;
    _size = 0;
    entry_allocated = 1000;

    state_table = (state_t **) malloc(sizeof(state_t *) * entry_allocated);
    for(int i = 0; i < entry_allocated; i++){
        state_table[i] = (state_t *) malloc(sizeof(state_t) * 16);
        memset(state_table, NO_STATE, sizeof(state_t) * 16);
    }
    is_accept = (int *) malloc(sizeof(int) * entry_allocated);
    memset(is_accept, 0, sizeof(int) * entry_allocated);
}

Fb_DFA::~Fb_DFA() {
    delete is_accept;
    for(int s = 0; s < _size; s++) delete state_table[s];
    delete state_table;
}

void Fb_DFA::add_transition(state_t s, int c, state_t d) {
    state_table[s][c] = d;
}
/*Brzozowski’s algorithm
 * step1: reverse edges & accept states and start states --> DFA to NFA-r
 * step2: powerset construction --> NFA-r to DFA-r
 * step3: repeat step 1 and step2 once
 * */
Fb_DFA* Fb_DFA::minimise() {
    //Fb_NFA* fb_nfa = this->reverse2nfa();
    Fb_NFA* fb_nfa = reverse2nfa(this);
    //fb_nfa->to_dot("./4nfa-1r.dot", "4nfa"); //debug
    Fb_DFA* fb_dfa = fb_nfa->nfa2dfa();
    //fb_dfa->to_dot("./4dfa-1r.dot", "4dfa");
    delete fb_nfa;
    if(fb_dfa == NULL) return NULL;
    //fb_nfa =  fb_dfa->reverse2nfa();
    fb_nfa = reverse2nfa(fb_dfa);
    delete fb_dfa;
    Fb_DFA* minimum_dfa = fb_nfa->nfa2dfa();
    delete fb_nfa;
    
    return minimum_dfa;
}

/* 1.reverse accept states and start states
 * 2.reverse edges
 * */
/*Fb_NFA* Fb_DFA::reverse2nfa() {
    Fb_NFA* nfa = new Fb_NFA(_size);
    //1.reverse accept states and start states


    //2.reverse edges
}*/

void Fb_DFA::to_dot(char *fname, char *title) {
    FILE* file = fopen(fname, "w");
    fprintf(file, "digraph \"%s\" {\n", title);

    for (state_t s=0;s<_size;s++){
        if(!is_accept[s])
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
        for(int i=0;i<csize;i++) mark[i]=0;
        for (int c=0;c<csize;c++){
            if (!mark[c]){
                mark[c]=1;
                if (state_table[s][c]!=0){
                    target=state_table[s][c];
                    label=(char *)malloc(100);
                    sprintf(label, "%02d", c);
                    bool range=true;
                    int begin_range=c;
                    for(int d=c+1;d<csize;d++){
                        if (state_table[s][d]==target){
                            mark[d]=1;
                            if (!range){
                                sprintf(temp, "%02d", d);
                                label=strcat(label,",");
                                label=strcat(label,temp);
                                begin_range=d;
                                range=1;
                            }
                        }
                        if (range && (state_table[s][d]!=target || d==csize-1)){
                            range=false;
                            if(begin_range!=d-1){
                                if (state_table[s][d]!=target)
                                {
                                    sprintf(temp, "%02d", d-1);
                                }
                                else
                                {
                                    sprintf(temp, "%02d", d);
                                }
                                label=strcat(label,"-");
                                label=strcat(label,temp);
                            }
                        }
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

int Fb_DFA::size() {
    return _size;
}

int Fb_DFA::less2states() {
    int res = 0;
    for(int s = 0; s < _size; s++){
        set<state_t> states;
        for(int c = 0; c < 16; c++) states.insert(state_table[s][c]);
        if(states.size() <= 2) res++;
        /*int cnt_states = 0;
        for(int c = 0; c < 16; c++){
            bool flag = false;
            for(int c_pre = 0; c_pre < c; c++){
                if(state_table[s][c] == state_table[s][c_pre]){
                    flag = true;
                    break;
                }
            }
            if(!flag) cnt_states++;
            if(cnt_states > 2) break;
        }
        if(cnt_states <= 2) res++;*/
    }
    return res;
}

int Fb_DFA::cons2states() {
    if(cons2state_num != -1) return cons2state_num;
    //1.less or equal 2 states
    //2.less than or euqal to 3 segments
    int res = 0;
    for(int s = 0; s < _size; s++){
        set<state_t> states;
        for(int c = 0; c < 16; c++) states.insert(state_table[s][c]);
        if(states.size() > 2) continue;

        //2.less than 3 segments
        int seg_number = 0;
        int beg = 0;
        while(beg + 1 < 16){
            int end;
            for(end = beg + 1; end < 16; end++){
                if(state_table[s][end] != state_table[s][beg]){
                    seg_number++;
                    beg = end;
                    break;
                }
            }
            if(end == 16) break;
        }
        seg_number++;
        if(seg_number <= 3) res++;
    }
    cons2state_num = res;
    return res;
}

Fb_DFA *Fb_DFA::converge(Fb_DFA *dfa) {
    /*1.filter: assume convergence will definitely cause a increase of state num*/
    if(this->size() + dfa->size() > 256) return nullptr;
    int big_states_num = (this->size() - this->cons2states()) + (dfa->size() - dfa->cons2states());
    if(big_states_num > 137) return nullptr;

    /*2.merge 2 dfas*/
    Fb_DFA* new_dfa = new Fb_DFA();
    list<state_t> queue;
    list<pair<state_t, state_t>> mapping_queue;
    map<pair<state_t, state_t>, state_t> mapping;

    state_t start = new_dfa->add_state();
    queue.push_back(start);
    mapping_queue.push_back(make_pair(0, 0));
    mapping[make_pair(0, 0)] = start;
    /*todo: ACCEPTING*/
    if(is_accept[0] || dfa->is_accept[0]) new_dfa->is_accept[start] = 1;

    while(!queue.empty()){
        state_t cur_state = queue.front(); queue.pop_front();
        pair<state_t, state_t> cur_pair = mapping_queue.front(); mapping_queue.pop_front();
        /*todo: ACCEPTING*/
        if(is_accept[cur_pair.first] || dfa->is_accept[cur_pair.second]) new_dfa->is_accept[cur_state] = 1;

        for(int c = 0; c < 16; c++){
            pair<state_t, state_t> destination_pair(state_table[cur_pair.first][c], dfa->state_table[cur_pair.second][c]);
            map<pair<state_t, state_t>, state_t>::iterator it = mapping.find(destination_pair);
            state_t next_state;
            if(it == mapping.end()){
                next_state = new_dfa->add_state();
                if(next_state > 256){
                    delete new_dfa;
                    return nullptr;
                }
                queue.push_back(next_state);
                mapping_queue.push_back(destination_pair);
                mapping[destination_pair] = next_state;
            }
            else{
                next_state = it->second;
            }
            new_dfa->add_transition(cur_state, c, next_state);
        }
    }

    /*Accuratly filter*/
    if(!new_dfa->small_enough()){
        delete new_dfa;
        return nullptr;
    }

    return new_dfa;
}

/*
 * 1.at most 256 states
 * 2.at most 137 big states(cannot be compressed)
 * */
bool Fb_DFA::small_enough() {
    if(size() > 256 || size() - cons2states() > 137){
        //fprintf(stderr, "dfa cannot be put into FPGA alone!\n");
        return false;
    }
    return true;
}

int Fb_DFA::get_large_states_num() {
    return this->size() - this->cons2states();
}

float Fb_DFA::get_ratio() {
    //todo
    if(small_enough()) return 0;

    int large_states_num = get_large_states_num();
    float times = (large_states_num - 120) * 1.0 / 120; //tigten 137 to 120
    float ratio1 = times / (times + 1);

    times = (size() - 220) * 1.0 / 220; //tigten 256 to 220
    float ratio2 = times / (times + 1);

    return max(ratio1, ratio2);
}
