//
// Created by 钟金诚 on 2020/5/25.
//

#ifndef BECCHI_REGEX_SET_H
#define BECCHI_REGEX_SET_H

#include "stdinc.h"

typedef struct _link_s{
    state_t id;
    struct _link_s *pre, *next;
}link_s;

typedef struct _daveset{
    //todo
    link_s *state_link;
    link_s *head;//hm change
}daveset;

#define FOREACH_DAVESET(set, it) \
    for(link_s *it=set->head; it!=NULL; it=it->next)

daveset *set_new();

int set_empty(daveset *s);

void set_insert(daveset *s, state_t state);

state_t set_begin(daveset *s);

void set_pop(daveset *s);

void set_rebegin(daveset *s);

void set_erase(daveset *s);

void set_delete(daveset *s);

daveset* set_copy(daveset *s);

#endif //BECCHI_REGEX_SET_H
