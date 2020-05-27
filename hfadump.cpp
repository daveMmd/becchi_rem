//
// Created by 钟金诚 on 2020/5/25.
//
/*
 * 测试hfadump正确性，以及实现匹配
 * */

#include "hfadump.h"
#include "set.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static char* dfa_mem;
static char* nfa_mem;
static char* nfaset_mem;
static char* node_mem;

int read_mem(char* fname){
    FILE* fdump = fopen(fname, "r");
    int dfa_size, nfa_size, nfaset_size, node_size;

    fscanf(fdump, "nfa_mem %d\n", &nfa_size);
    nfa_mem = (char*) malloc(sizeof(nfa_mem_block) * nfa_size);
    fread(nfa_mem, sizeof(nfa_mem_block), nfa_size, fdump);

    fscanf(fdump, "dfa_mem %d\n", &dfa_size);
    //fread(&dfa_size, 1, 4, fdump);
    dfa_mem = (char*) malloc(sizeof(dfa_mem_block) * dfa_size);
    fread(dfa_mem, sizeof(dfa_mem_block), dfa_size, fdump);

    fscanf(fdump, "nfaset_mem %d\n", &nfaset_size);
    nfaset_mem = (char *) malloc(sizeof(unsigned int) * nfaset_size);
    fread(nfaset_mem, sizeof(unsigned int), nfaset_size, fdump);

    fscanf(fdump, "node_mem %d\n", &node_size);
    node_mem = (char *) malloc(sizeof(node_set_app_id_and_aging) * node_size);
    fread(node_mem, sizeof(node_set_app_id_and_aging), node_size, fdump);

    fclose(fdump);
}

#if 1 //array stores nfa sets
#define MAX_ACTIVE_NFA_STATES 1000
state_t array1[MAX_ACTIVE_NFA_STATES];
state_t array2[MAX_ACTIVE_NFA_STATES];
int mem_search_hfa(unsigned char* str, int n)
{
    node_set_app_id_and_aging *accept_data;

    state_t dfastate = 0;
    dfa_mem_block *block=(dfa_mem_block *)dfa_mem; //dfa current state
    state_t *nfa_currentstates = array1;
    state_t *nfa_nextstates = array2;
    int currentstates_size = 0;
    int nextstates_size = 0;
    //dfa_mem_block *rewind_block=dfa_mem+sizeof(dfa_mem_block);
    unsigned char *start=str;
    unsigned char *end=str+n;
    unsigned int to;
    unsigned char *c=start;
    unsigned int from=0;
    unsigned short curptr=0;
    while(c < end)
    {
        //head DFA traverse
        if( (char *)block == dfa_mem)
        {
            from=curptr;
        }

        dfastate = block->nextstates[*c];
        //block=(dfa_mem_block *)(dfa_mem+block->nextstates[*c]);
        block = ((dfa_mem_block *)dfa_mem) + dfastate;
        if(block->stateaccepts.accept_num>0)
        {
            //accept_data=(node_set_app_id_and_aging *)(node_mem+block->stateaccepts.accept_offset);
            accept_data= ((node_set_app_id_and_aging *)node_mem) + block->stateaccepts.accept_offset;
            for(int i=0;i<block->stateaccepts.accept_num;i++)
            {
                //onevent(accept_data->app_id,from,to,1,ctxt);
                printf("matching rule %d at position %d.\n", accept_data->app_id, curptr);
                accept_data++;
            }
        }

        //tail-NFA traverse
        //nfa_nextstates = set_new();
        nextstates_size = 0;
        //traverse next states
        for(int i=0; i<currentstates_size; i++){
            state_t ns = nfa_currentstates[i];
            int offset = ((nfa_mem_block*) nfa_mem[ns])->nextstates_offset[*c];
            if(offset == VALUE_NULL) continue;
            while(((int*)nfaset_mem)[offset] != VALUE_END)
            {
                bool flag = false;
                for(int j=0; j<nextstates_size; j++){
                    if(nfa_nextstates[j] == ((int*)nfaset_mem)[offset]){
                        flag = true;
                        break;
                    }
                }
                assert(nextstates_size < MAX_ACTIVE_NFA_STATES-1);
                if(!flag) nfa_nextstates[nextstates_size++] = ((int*)nfaset_mem)[offset];
                offset++;
            }
        }

        //insert border state if needed
        if(block->isborder != VALUE_NULL){
            int offset = block->offset_to_nfaset;
            while(((int*)nfaset_mem)[offset] != VALUE_END)
            {
                bool flag = false;
                for(int j=0; j<nextstates_size; j++){
                    if(nfa_nextstates[j] == ((int*)nfaset_mem)[offset]){
                        flag = true;
                        break;
                    }
                }
                assert(nextstates_size < MAX_ACTIVE_NFA_STATES-1);
                if(!flag) nfa_nextstates[nextstates_size++] = ((int*)nfaset_mem)[offset];
                offset++;
            }
        }

        //check nfa states accept rules
        for(int i=0; i<nextstates_size; i++){
            nfa_mem_block* nfablock = ((nfa_mem_block*)nfa_mem) + nfa_nextstates[i];
            accept_data= ((node_set_app_id_and_aging *)node_mem) + nfablock->stateaccepts.accept_offset;
            for(int i=0; i<nfablock->stateaccepts.accept_num; i++)
            {
                //onevent(accept_data->app_id,from,to,1,ctxt);
                printf("matching rule %d at position %d.\n", accept_data->app_id, curptr);
                accept_data++;
            }
        }
        //swap the array of next states and current states
        currentstates_size = nextstates_size;
        state_t *temp = nfa_currentstates;
        nfa_currentstates = nfa_nextstates;
        nfa_nextstates = temp;

        //next character
        curptr++;
        c=start+curptr;
    }

    return 0;
}
#else //link-set representing NFA sets
int mem_search_hfa(unsigned char* str, int n)
{
    node_set_app_id_and_aging *accept_data;

    state_t dfastate = 0;
    dfa_mem_block *block=(dfa_mem_block *)dfa_mem; //dfa current state
    daveset* nfa_currentstates = set_new();
    daveset* nfa_nextstates;
    //dfa_mem_block *rewind_block=dfa_mem+sizeof(dfa_mem_block);
    unsigned char *start=str;
    unsigned char *end=str+n;
    unsigned int to;
    unsigned char *c=start;
    unsigned int from=0;
    unsigned short curptr=0;
    while(c < end)
    {
        //head DFA traverse
        if( (char *)block == dfa_mem)
        {
            from=curptr;
        }

        dfastate = block->nextstates[*c];
        //block=(dfa_mem_block *)(dfa_mem+block->nextstates[*c]);
        block = ((dfa_mem_block *)dfa_mem) + dfastate;
        if(block->stateaccepts.accept_num>0)
        {
            //accept_data=(node_set_app_id_and_aging *)(node_mem+block->stateaccepts.accept_offset);
            accept_data= ((node_set_app_id_and_aging *)node_mem) + block->stateaccepts.accept_offset;
            for(int i=0;i<block->stateaccepts.accept_num;i++)
            {
                //onevent(accept_data->app_id,from,to,1,ctxt);
                printf("matching rule %d at position %d.\n", accept_data->app_id, curptr);
                accept_data++;
            }
        }

        //tail-NFA traverse
        nfa_nextstates = set_new();
        //traverse next states
        FOREACH_DAVESET(nfa_currentstates, it){
            state_t ns = it->id;
            int offset = ((nfa_mem_block*) nfa_mem[ns])->nextstates_offset[*c];
            if(offset == VALUE_NULL) continue;
            while(((int*)nfaset_mem)[offset] != VALUE_END)
            {
                set_insert(nfa_nextstates, ((int*)nfaset_mem)[offset]);
                offset++;
            }
        }

        set_delete(nfa_currentstates);
        nfa_currentstates = nfa_nextstates;
        //insert border state if needed
        if(block->isborder != VALUE_NULL){
            int offset = block->offset_to_nfaset;
            while(((int*)nfaset_mem)[offset] != VALUE_END)
            {
                set_insert(nfa_nextstates, ((int*)nfaset_mem)[offset]);
                offset++;
            }
        }

        //check nfa states accept rules
        FOREACH_DAVESET(nfa_nextstates, it){
            nfa_mem_block* nfablock = ((nfa_mem_block*)nfa_mem) + it->id;
            accept_data= ((node_set_app_id_and_aging *)node_mem) + nfablock->stateaccepts.accept_offset;
            for(int i=0; i<nfablock->stateaccepts.accept_num; i++)
            {
                //onevent(accept_data->app_id,from,to,1,ctxt);
                printf("matching rule %d at position %d.\n", accept_data->app_id, curptr);
                accept_data++;
            }
        }

        //next character
        curptr++;
        c=start+curptr;
    }

    return 0;
}
#endif