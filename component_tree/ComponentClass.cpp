//
// Created by 钟金诚 on 2020/8/13.
//

#include "ComponentClass.h"

void ComponentClass::insert(int pos) {
    charReach.set(pos);
}

void ComponentClass::negate() {
    charReach.flip();
}

void ComponentClass::insert(int start, int end) {
    for(int c = start; c <= end; c++) charReach.set(c);
}

int ComponentClass::head() {
    for(int c = 0; c < 256; c++)
        if(charReach[c]) return c;
}

int ComponentClass::num_concat() {
    return 1;
}

char *ComponentClass::get_re_part() {
    return re_part;
    //return nullptr;
}

bool ComponentClass::decompose(int &threshold, std::bitset<256> &alpha, char *R_pre, char *R_post, int& depth, std::bitset<256> *first_charClass, std::bitset<256> *last_infinite_charclass) {
    if(threshold <= 0) return true;
    if((charReach&(*last_infinite_charclass)).any()) *first_charClass = (*first_charClass)|charReach;
    last_infinite_charclass->reset();

    threshold -= 1;
    strcat(R_pre, get_re_part());

    //update depth
    depth++;
    return false;
}

char *ComponentClass::get_reverse_re() {
    return get_re_part();
}
