//
// Created by 钟金诚 on 2020/7/22.
//

#include <vector>
#include <bitset>
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
    accept_to_prefix_dfa = NULL;
    dead_state = NO_STATE;
    cons2state_num = -1;
    smallsate_num = -1;
    _size = 17 * dfa->size();// each node plus 16 new nodes --> 17 nodes
    state_table = (state_t **) malloc(sizeof(state_t *) * (_size + 5));
    for(int i = 0; i < _size + 5; i++){
        state_table[i] = (state_t *) malloc(sizeof(state_t) * 16);
    }
    is_accept = (int *) malloc(sizeof(int) * _size);
    memset(is_accept, 0, sizeof(int) * _size);

    //accept states && set dead_state
    for(state_t s = 0; s < dfa->size(); s++){
        if(!dfa->accepts(s)->empty()){
            is_accept[s*17] = 1;
        }
        if(s == dfa->dead_state) dead_state = s * 17;
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
            state_table[s]=nullptr;
        }

        is_accept = reallocate_int_array(is_accept, entry_allocated);
    }
    state_table[state] = allocate_state_array(16);
    int i; for (i=0;i<16;i++) state_table[state][i]=NO_STATE;

    return state;
}

Fb_DFA::Fb_DFA() {
    accept_to_prefix_dfa = nullptr;
    dead_state = NO_STATE;
    cons2state_num = -1;
    smallsate_num = -1;
    _size = 0;
    entry_allocated = 1000;

    state_table = (state_t **) malloc(sizeof(state_t *) * entry_allocated);
    for(int i = 0; i < entry_allocated; i++){
        state_table[i] = nullptr;
        /*state_table[i] = (state_t *) malloc(sizeof(state_t) * 16);
        memset(state_table[i], NO_STATE, sizeof(state_t) * 16);*/
    }
    is_accept = (int *) malloc(sizeof(int) * entry_allocated);
    memset(is_accept, 0, sizeof(int) * entry_allocated);
}

Fb_DFA::~Fb_DFA() {
    free(is_accept);
    for(int s = 0; s < _size; s++) free(state_table[s]);
    free(state_table);
    if(accept_to_prefix_dfa != nullptr){
        for(int s = 0; s < _size; s++) delete accept_to_prefix_dfa[s];
    }
    free(accept_to_prefix_dfa);
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
            //fprintf(file, " %ld [shape=circle,label=\"%ld-%ld\"];\n", s, s/17, s%17);
            fprintf(file, " %u [shape=circle,label=\"%u\"];\n", s, s);
        else
            //fprintf(file, " %ld [shape=doublecircle,label=\"%ld-%ld\"];\n", s, s/17, s%17);
            fprintf(file, " %u [shape=doublecircle,label=\"%u\"];\n", s, s);
    }
    int csize = 16;
    int *mark=allocate_int_array(csize);
    char *label= nullptr;
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
            if (label!= nullptr) {
                fprintf(file, "%u -> %u [label=\"%s\"];\n", s,target,label);
                free(label);
                label= nullptr;
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

void try_insert_new_accept(Fb_DFA *dfa1, Fb_DFA *dfa2, Fb_DFA *newdfa, state_t s1, state_t s2, state_t news){
    if(!dfa1->is_accept[s1] && !dfa2->is_accept[s2]) return;
    newdfa->is_accept[news] = 1;
    newdfa->accept_to_prefix_dfa[news] = new list<uint16_t >();
    list<uint16_t >* newa = newdfa->accept_to_prefix_dfa[news];
    list<uint16_t >* a1 = dfa1->accept_to_prefix_dfa[s1];
    list<uint16_t >* a2 = dfa2->accept_to_prefix_dfa[s2];
    if(a1 != nullptr) newa->insert(newa->end(), a1->begin(), a1->end());
    if(a2 != nullptr) newa->insert(newa->end(), a2->begin(), a2->end());
}

/*dfa will not change*/
void init_accept_rules(Fb_DFA* dfa){
    if(dfa->accept_to_prefix_dfa == nullptr){
        dfa->accept_to_prefix_dfa = (list<uint16_t >**) malloc(sizeof(list<uint16_t>*) * dfa->_size);
        for(state_t s = 0; s < dfa->_size; s++) {
            if(dfa->is_accept[s]) {
                dfa->accept_to_prefix_dfa[s] = new list<uint16_t>();
                dfa->accept_to_prefix_dfa[s]->push_back(dfa->ind_prefixdfa_single);
            }
            else dfa->accept_to_prefix_dfa[s] = nullptr;
        }
    }
}

Fb_DFA *Fb_DFA::converge(Fb_DFA *dfa) {
    /*1.filter: assume convergence will definitely cause a increase of state num*/
    if(this->size() + dfa->size() > MAX_STATES_NUMBER*1.2) return nullptr;
    int big_states_num = this->get_large_states_num() + dfa->get_large_states_num();//(this->size() - this->cons2states()) + (dfa->size() - dfa->cons2states());
    if(big_states_num > MAX_BIG_SATES_NUMBER*1.2) return nullptr;

    //init the two dfa`s accept_rule_list
    init_accept_rules(this);
    init_accept_rules(dfa);

    /*2.merge 2 dfas*/
    Fb_DFA* new_dfa = new Fb_DFA();
    new_dfa->accept_to_prefix_dfa = (list<uint16_t >**) malloc(sizeof(list<uint16_t>*) * new_dfa->entry_allocated);
    for(state_t s=0; s < entry_allocated; s++) new_dfa->accept_to_prefix_dfa[s] = nullptr;
    list<state_t> queue;
    list<pair<state_t, state_t>> mapping_queue;
    map<pair<state_t, state_t>, state_t> mapping;

    state_t start = new_dfa->add_state();
    queue.push_back(start);
    mapping_queue.push_back(make_pair(0, 0));
    mapping[make_pair(0, 0)] = start;
    /*todo: ACCEPTING*/
    try_insert_new_accept(this, dfa, new_dfa, 0, 0, 0);

    while(!queue.empty()){
        state_t cur_state = queue.front(); queue.pop_front();
        pair<state_t, state_t> cur_pair = mapping_queue.front(); mapping_queue.pop_front();
        /*todo: ACCEPTING*/
        //if(is_accept[cur_pair.first] || dfa->is_accept[cur_pair.second]) new_dfa->is_accept[cur_state] = 1;
        try_insert_new_accept(this, dfa, new_dfa, cur_pair.first, cur_pair.second, cur_state);

        for(int c = 0; c < 16; c++){
            pair<state_t, state_t> destination_pair(state_table[cur_pair.first][c], dfa->state_table[cur_pair.second][c]);
            map<pair<state_t, state_t>, state_t>::iterator it = mapping.find(destination_pair);
            state_t next_state;
            if(it == mapping.end()){
                next_state = new_dfa->add_state();
                if(next_state > MAX_STATES_NUMBER){
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

    //minimise, time-consuming
    Fb_DFA* minimise_dfa = new_dfa;//= new_dfa->minimise2();
    //delete new_dfa;

    /*Accuratly filter*/
    if(!minimise_dfa->small_enough()){
        delete minimise_dfa;
        return nullptr;
    }

    return minimise_dfa;
}

/*
 * 1.at most MAX_STATES_NUMBER states
 * 2.at most MAX_BIG_SATES_NUMBER big states(cannot be compressed)
 * */
bool Fb_DFA::small_enough() {
    return !(size() > MAX_STATES_NUMBER || get_large_states_num() > MAX_BIG_SATES_NUMBER);
}

int Fb_DFA::get_large_states_num() {
    //return this->size() - this->cons2states();
    return this->size() - this->get_smallstate_num();
}

float Fb_DFA::get_ratio() {
    //todo
    if(small_enough()) return 0;

    int large_states_num = get_large_states_num();
    float times = (large_states_num - MAX_BIG_SATES_NUMBER + 5) * 1.0 / (MAX_BIG_SATES_NUMBER - 5); //tigten MAX_BIG_SATES_NUMBER to MAX_BIG_SATES_NUMBER - 5
    float ratio1 = times / (times + 1);

    times = (size() - MAX_STATES_NUMBER + 5) * 1.0 / (MAX_STATES_NUMBER - 5); //tigten MAX_STATES_NUMBER to MAX_STATES_NUMBER - 5
    float ratio2 = times / (times + 1);

    return max(ratio1, ratio2);
}

void add_belong(state_t state, int partition, int *partition_belong, vector<list<state_t> *> *v){
    partition_belong[state] = partition;
    while(v->size() < partition+1) {
        auto * new_list = new list<state_t >();
        v->push_back(new_list);
    }
    (*v)[partition]->push_back(state);
}

Fb_DFA *Fb_DFA::minimise2() {
    int *partition_belong1 = (int*) malloc(sizeof(int) * (_size + 5));
    int *partition_belong2 = (int*) malloc(sizeof(int) * (_size + 5));
    bool has_distinguish = true; //record Pk == Pk-1
    vector<list<state_t> *> v1;
    vector<list<state_t> *> v2;

    /*step1. partition final states and non-final states and get P0*/
    for(state_t state = 0; state < _size; state++){
        if(is_accept[state]) add_belong(state, 0, partition_belong1, &v1);
        else add_belong(state, 1, partition_belong1, &v1);
        //else partition_belong1[state] = 1;
    }

    /*step2. repeat partition state set unless no new distinguishable states are generated*/
    int* partition_pre = partition_belong1;
    int* partition_next = partition_belong2;
    vector<list<state_t> *> *v_pre = &v1;
    vector<list<state_t> *> *v_next = &v2;
    int *vis = (int *) malloc(sizeof(int) * (_size + 5)); // used to cut some pari to accelerate
    while(has_distinguish){
        /*init each repeat*/
        has_distinguish = false;
        int new_partition = 0;
        memset(vis, 0, sizeof(int) * (_size + 5));
        //free v_next
        for(auto& it: *v_next) delete it;
        v_next->clear();

        //iterate each partition
        for(auto &lis : *v_pre){
            //iterate each pair of states
            for(auto it_i = lis->begin(); it_i != lis->end(); it_i++){
                bool flag_new_partition = true;
                //for(auto it_j = it_i; it_j != lis->begin(); it_j--){
                if(it_i != lis->begin()) {
                    for (auto it_j = it_i;; it_j--) {
                        if (it_j == it_i) continue;
                        if (vis[*it_j]) continue;

                        bool flag_distinguish = false;
                        for (int c = 0; c < 16; c++) {
                            if (partition_pre[get_next_state(*it_i, c)] != partition_pre[get_next_state(*it_j, c)]) {
                                flag_distinguish = true;
                                break;
                            }
                        }

                        /*judge if *it_i is distinguishable with *it_j*/
                        if (flag_distinguish) has_distinguish = true;
                        else { //add to the same partition as *it_j(state)
                            vis[*it_i] = 1;//can be cut
                            flag_new_partition = false;
                            add_belong(*it_i, partition_next[*it_j], partition_next, v_next);
                            break;
                        }

                        if (it_j == lis->begin()) break;
                    }
                }
                //*it_j(state) not in the same partition with any former state -> generate new partiton
                if(flag_new_partition) {
                    add_belong(*it_i, new_partition, partition_next, v_next);
                    new_partition += 1;
                }
            }
        }

        //swap pre and next
        swap(partition_pre, partition_next);
        swap(v_pre, v_next);
    }

    /*step3. each partition is equivalent to one new state of reduced DFA*/
    auto reduced_dfa = new Fb_DFA();
    /*gen state*/
    int *partition_state_map = partition_belong1;
    for(int i = 0; i < (_size + 5); i++) partition_state_map[i] = -1;
    reduced_dfa->add_state(); //gen state 0
    //find which partition contain state 0 && set dead_state && set accept state
    for(int i = 0; i < v_next->size(); i++){
        list<state_t>* lis = (*v_next)[i];
        for(auto &state: *lis){
            if(state == 0) {
                partition_state_map[i] = 0;
            }
            if(dead_state != NO_STATE && state == dead_state) {
                if(partition_state_map[i] == -1){
                    reduced_dfa->dead_state = reduced_dfa->add_state();
                    partition_state_map[i] = reduced_dfa->dead_state;
                }
                else reduced_dfa->dead_state = partition_state_map[i];
            }
            if(is_accept[state]){
                if(partition_state_map[i] == -1){
                    state_t new_state = reduced_dfa->add_state();
                    reduced_dfa->is_accept[new_state] = 1;
                    partition_state_map[i] = new_state;
                }
                else reduced_dfa->is_accept[partition_state_map[i]] = 1;
            }

            //need adjust partition_belong2?
            partition_belong2[state] = i;
        }
    }

    //gen normal states
    for(int i = 0; i < v_next->size(); i++){
        if(partition_state_map[i] != -1) continue;
        state_t state = reduced_dfa->add_state();
        partition_state_map[i] = state;
    }

    /*gen transition*/
    for(int i = 0; i < v_next->size(); i++){
        state_t source = (*v_next)[i]->front();
        int partition_source = partition_belong2[source];
        for(int c = 0; c < 16; c++){
            state_t target = get_next_state(source, c);
            int partition_target = partition_belong2[target];
            reduced_dfa->add_transition(partition_state_map[partition_source], c, partition_state_map[partition_target]);
        }
    }

    //free resources
    free(partition_belong1);
    free(partition_belong2);
    free(vis);
    //free v_next
    for(auto& it: v1) delete it;
    for(auto& it: v2) delete it;

    return reduced_dfa;
}

state_t Fb_DFA::get_next_state(state_t state, int c) {return state_table[state][c]; }

//less than or equal to two target states
int Fb_DFA::get_smallstate_num() {
    if(smallsate_num != -1) return smallsate_num;
    int cnt = 0;
    for(int s = 0; s < _size; s++){
        set<state_t> states;
        states.clear();
        for(int c = 0; c < 16; c++) states.insert(state_table[s][c]);
        if(states.size() <= 2) cnt++;
    }
    smallsate_num = cnt;
    return cnt;
}

void *bits2ptr(bitset<20480> &bits){
    char* mem = (char *)malloc(sizeof(char)*5*512);
    for(int i=0; i<20480/8; i++){
        char tem = 0;
        for(int j = 0; j < 8; j++){
            tem = tem | (bits[8*i+j] << (7-j));
        }
        mem[i] = tem;
    }
    return mem;
}

/*
 * 前100个状态用classical编码，后112个状态用bitmap编码 （100*4 + 112 == 512）
 *
 * */
void* Fb_DFA::to_BRAM(uint16_t dfaid, map<uint32_t, list<uint16_t>* > *mapping_table) {
    if(size() > MAX_STATES_NUMBER || get_large_states_num() > MAX_BIG_SATES_NUMBER){
        fprintf(stderr, "Error: fb_dfa size too large (cannot be placed into BRAM)\n");
        return nullptr;
    }

    /*state renumber (put big states in front)*/
    int big_num = 0;
    int small_num = get_large_states_num();
    int map[256];
    for(state_t state=0; state<_size; state++){
        if(is_bigstate(state)) map[state] = big_num++;
        else map[state] = small_num++;
    }

    //refresh accept table and transition table
    int new_accept[256];
    int new_statetable[256][16];
    for(state_t state=0; state<_size; state++){
        state_t newstate = map[state];
        if(is_accept[state])
        {
            new_accept[newstate] = 1;
            /*记录state renumber之后mapping table*/
            uint32_t key = (dfaid * MAX_STATES_NUMBER) + newstate;
            //auto lis = new list<uint16_t>();
            //lis->insert(lis->begin(), accept_to_prefix_dfa[state]->begin(), accept_to_prefix_dfa[state]->end());
            (*mapping_table)[key] = accept_to_prefix_dfa[state]; //指针易被释放 //lis;
        }
        else new_accept[newstate] = 0;

        for(int c=0; c<16; c++)
        {
            state_t newtarget = map[state_table[state][c]];
            new_statetable[newstate][c] = newtarget;
        }
    }
    /*generate m20k memory*/
    bitset<20480> bits;
    int cur = 0;
    for(state_t state = 0; state < _size; state++){
        if(state < MAX_BIG_SATES_NUMBER){
            /*classical encoding*/
            for(int c=0; c<16; c++){
                state_t target = new_statetable[state][c];
                for(int i = 0; i < 8; i++){
                    if(target & (1<<(7-i))) bits[cur++] = 1;
                    else bits[cur++] = 0;
                }
                if(new_accept[target]) bits[cur++] = 1;
                else bits[cur++] = 0;
                //补齐40位
                if((c+1) % 4 == 0) cur += 4;
            }
        }
        else{
            /*bitmap encoding*/
            state_t target1 = new_statetable[state][0];
            bits[cur++] = 0;
            state_t target2 = NO_STATE;
            //write bitmap
            for(int c=1; c<16; c++){
                if(new_statetable[state][c] == target1) bits[cur++] = 0;
                else{
                    target2 = new_statetable[state][c];
                    bits[cur++] = 1;
                }
            }
            //write target1
            for(int i = 0; i < 8; i++){
                if(target1 & (1<<(7-i))) bits[cur++] = 1;
                else bits[cur++] = 0;
            }
            if(new_accept[target1]) bits[cur++] = 1;
            else bits[cur++] = 0;

            //write target2
            for(int i = 0; i < 8; i++){
                if(target2 & (1<<(7-i))) bits[cur++] = 1;
                else bits[cur++] = 0;
            }
            if(target2 != NO_STATE && new_accept[target2]) bits[cur++] = 1;
            else bits[cur++] = 0;

            //补齐40位
            cur += 6;
        }
    }
    return bits2ptr(bits);
}

bool Fb_DFA::is_bigstate(state_t s) {
    set<state_t> states;
    states.clear();
    for(int c = 0; c < 16; c++) states.insert(state_table[s][c]);
    if(states.size() <= 2) return false;
    return true;
};
