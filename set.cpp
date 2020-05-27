//
// Created by 钟金诚 on 2020/5/25.
//
#include "set.h"
#include "stdinc.h"

daveset *set_new(){
    daveset *s = (daveset*) malloc(sizeof(daveset));
    memset(s, 0, sizeof(daveset));
    return s;
}
int set_empty(daveset *s){
    //todo
    if(s->state_link == NULL) return 1;
    return 0;
}

void set_insert(daveset *s, state_t state){
    //link_s *cur=s->state_link;//hm change
    link_s *cur=s->head;
    //to last (id < state) ptr
    if(cur == NULL){
        link_s* newnode = (link_s*) malloc(sizeof(link_s));
        s->state_link = newnode;
        s->head = newnode;//hm change
        newnode->pre = NULL;
        newnode->next = NULL;
        newnode->id = state;
        return;
    }

    if(cur->id == state) return;
    while(cur->next !=NULL && cur->next->id < state){
        cur = cur->next;
    }
    if(cur->next == NULL){//tail
        link_s* newnode = (link_s*) malloc(sizeof(link_s));
        cur->next = newnode;
        newnode->pre = cur;
        newnode->id = state;
        newnode->next = NULL;
    }
    else if (cur->next->id == state){//find the same element
        return;
    }
    else if(cur->pre == NULL){//head
        link_s* newnode = (link_s*) malloc(sizeof(link_s));
        newnode->pre = NULL;
        newnode->id = state;
        newnode->next = cur;
        s->state_link = newnode;
        s->head = newnode;//hm change
    }
    else{//middle
        link_s* newnode = (link_s*) malloc(sizeof(link_s));
        newnode->pre = cur;
        newnode->id = state;
        newnode->next = cur->next;
        cur->next = newnode;
    }
    //todo
}

state_t set_begin(daveset *s){
    if(s->state_link == NULL){
        printf("set empty!\n");
        exit(-1);
    }
    //while(s->state_link->pre != NULL) s->state_link = s->state_link->pre;
    return s->state_link->id;
    //todo
}

void set_rebegin(daveset *s){
    //while(s->state_link->pre != NULL) s->state_link = s->state_link->pre;
    s->state_link = s->head;
}
//set_erase 替代
void set_pop(daveset *s){
    if(s->state_link == NULL)
        return;
    s->state_link = s->state_link->next;
}

void set_erase(daveset *s){
    link_s * begin = s->state_link;
    if(begin == NULL) return;
    s->state_link = begin->next;
    if(s->state_link) s->state_link->pre = NULL;
    free(begin);
    //todo
}

void set_delete(daveset *s){
    if(s==NULL) return;
    link_s *node;
    //link_s * begin = s->state_link;//hm change
    link_s * begin = s->head;
    while(begin){
        node = begin;
        free(begin);
        begin = begin->next;
    }
    free(s);
}
daveset* set_copy(daveset *s){
    daveset *news = (daveset*) malloc(sizeof(daveset));
    memset(news, 0, sizeof(daveset));
    link_s *ls = s->state_link;
    while(ls){
        set_insert(news, ls->id);
        ls = ls->next;
    }
    return news;
}