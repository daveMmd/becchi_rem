//
// Created by 钟金诚 on 2020/5/25.
//

#ifndef BECCHI_REGEX_HFADUMP_H
#define BECCHI_REGEX_HFADUMP_H

#define VALUE_END 0xffffffff //indicating tail
#define VALUE_NULL 0xffffffff //indicating NULL

int read_mem(char* fname);

int mem_search_hfa(unsigned char* str, int n);

typedef struct  _accept_state
{
    unsigned int     accept_num:4;
    unsigned int     accept_offset:28;
}accept_state;

#define ALPHABET_SIZE 256

typedef union  _dfa_mem_block
{
    struct
    {
        unsigned int     nextstates[ALPHABET_SIZE];  /* fix size */
        accept_state     stateaccepts;
        union{
            unsigned int isborder;
            unsigned int offset_to_nfaset;
        };
    };
    unsigned int rawdata[258];
}__attribute__((packed)) dfa_mem_block;

typedef union  _nfa_mem_block
{
    struct
    {
        unsigned int     nextstates_offset[ALPHABET_SIZE];  /* fix size : nfastate 指向的nfaset 偏移*/
        accept_state     stateaccepts;
    };
    unsigned int rawdata[257];
}__attribute__((packed)) nfa_mem_block;

typedef struct _data_node
{
    unsigned char type;
    unsigned char flags;
}__attribute__((packed))data_node;

typedef struct
{
    union
    {
        data_node header;
        struct
        {
            unsigned char reserved;
            unsigned char action;
        };
    };
    unsigned short parser_type;
    unsigned short app_id;
    unsigned short aging;
}node_set_app_id_and_aging;

#endif //BECCHI_REGEX_HFADUMP_H
