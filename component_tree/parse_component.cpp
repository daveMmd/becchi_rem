//
// Created by 钟金诚 on 2020/8/13.
//

#include "parse_component.h"
#include "../stdinc.h"
#include "../parser.h"

ComponentClass* parse_escape(int &ptr, char* re){
    ComponentClass* compClass = new ComponentClass();
    int ptr_pre = ptr;
    ptr++;
    int char_value;
    switch(re[ptr]){
        case 'x':
        case 'X':
            if(ptr>strlen(re)-3)
                fatal((char*)"parse_escape: invalid hex escape sequence.");
            else if (!is_hex_digit(re[ptr+1]) || !is_hex_digit(re[ptr+2]))
                fatal((char*)"regex_parser::process_escape: invalid hex escape sequence.");
            else{
                char tmp[5];
                tmp[0]='0'; tmp[1]=re[ptr]; tmp[2]=re[ptr+1]; tmp[3]=re[ptr+2]; tmp[4]='\0';
                sscanf(tmp,"0x%x", &char_value);
                compClass->insert(char_value);
                ptr=ptr+3;
            }
            break;
        case 's':
            compClass->insert('\t');
            compClass->insert('\n');
            compClass->insert('\r');
            compClass->insert('\x0C');
            compClass->insert('\x20');
            ptr++;
            break;
        case 'S':
            compClass->insert('\t');
            compClass->insert('\n');
            compClass->insert('\r');
            compClass->insert('\x0C');
            compClass->insert('\x20');
            compClass->negate();
            ptr++;
            break;
        case 'w':
            compClass->insert('_');
            compClass->insert('0', '9');
            compClass->insert('a', 'z');
            compClass->insert('A', 'Z');
            ptr++;
            break;
        case 'W':
            compClass->insert('_');
            compClass->insert('0', '9');
            compClass->insert('a', 'z');
            compClass->insert('A', 'Z');
            compClass->negate();
            ptr++;
            break;
        case 'd':
            compClass->insert('0', '9');
            ptr++;
            break;
        case 'D':
            compClass->insert('0', '9');
            compClass->negate();
            ptr++;
            break;
        default:
            char_value = escaped(re[ptr]);
            compClass->insert(char_value);
            ptr++;
            break;
    }
    memcpy(compClass->re_part, re + ptr_pre, ptr - ptr_pre);
    compClass->re_part[ptr - ptr_pre] = '\0';
    return compClass;
}

ComponentClass* parse_range(int &ptr, char *re){
    int ptr_pre = ptr; //range start '['
    ptr++;
    if (re[ptr]==CLOSE_SBRACKET)
        fatal((char*)"parse_range: empty range.");
    bool negate=false;
    int from=CSIZE+1;
    //int_set *range=new int_set(CSIZE);
    ComponentClass* range = new ComponentClass();
    if(re[ptr]==TILDE){
        negate=true;
        ptr++;
    }
    while(ptr!=strlen(re)-1 && re[ptr]!=CLOSE_SBRACKET){
        symbol_t to = re[ptr];
        //if (is_special(to) && to!=ESCAPE)
        //	fatal("regex_parser:: process_range: invalid character.");
        if (to==ESCAPE){
            ComponentClass* tem_compClass = parse_escape(ptr, re);
            to = tem_compClass->head();
            delete tem_compClass;
        }
        else
            ptr++;
        if (from==(CSIZE+1)) from=to;
        if (ptr!=strlen(re)-1 && re[ptr]==MINUS_RANGE){
            ptr++;
        }else{
            if (from>to)
                fatal((char*)"parse_range: invalid range.");
            range->insert(from, to);
            from=CSIZE+1;
        }
    }
    if (re[ptr]!=CLOSE_SBRACKET)
        fatal((char*)"parse_range: range not closed.");
    ptr++;
    memcpy(range->re_part, re + ptr_pre, ptr - ptr_pre);
    range->re_part[ptr - ptr_pre] = '\0';

    if (negate) range->negate();

    return range;
}

void process_quantifier(const char *re, int &ptr, int *lb_p, int *ub_p){
    ptr++;
    int lb=0;
    int ub=_INFINITY;
    int res=sscanf(re+ptr,"%d",&lb);
    if (res!=1) fatal((char*)"process_quantifier: wrong quantified expression.");
    while(ptr!=strlen(re) && re[ptr]!=COMMA && re[ptr]!=CLOSE_QBRACKET)
        ptr++;
    if (ptr==strlen(re) || (re[ptr]==COMMA && ptr==strlen(re)-1))
        fatal((char*)"regex_parser:: process_quantifier: wrong quantified expression.");
    if(re[ptr]==CLOSE_QBRACKET){
        ub=lb;
    }else{
        ptr++;
        if(re[ptr]!=CLOSE_QBRACKET){
            res=sscanf(re+ptr,"%d}",&ub);
            if (res!=1) fatal((char*)"regex_parser:: process_quantifier: wrong quantified expression.");
        }
    }
    while(re[ptr]!=CLOSE_QBRACKET)
        ptr++;
    (*lb_p)=lb;
    (*ub_p)=ub;
    ptr++;
}

Component* parse_subcomp(int &ptr, char *re){
    ptr++;
    bool flag_anchor = false;
    ComponentSequence* compSeq = new ComponentSequence();
    Component* comp = NULL;
    ComponentAlternation* compAlter = NULL;

    if(re[ptr] == '^'){
        flag_anchor = true;
        ptr++;
    }

    while(ptr < strlen(re) && re[ptr] != ')'){
        switch(re[ptr]){
            case '\\':
                comp = parse_escape(ptr, re);
                break;
            case '.':
                ptr++;
                comp = new ComponentClass();
                sprintf(((ComponentClass*)comp)->re_part, ".");
                ((ComponentClass*) comp)->negate();
                break;
            case '[':
                comp = parse_range(ptr, re);
                break;
            case '(':
                comp = parse_subcomp(ptr, re);
                break;
            case '|':
                ptr++;
                if(compAlter == NULL) compAlter = new ComponentAlternation();
                compAlter->add(compSeq);
                compSeq = new ComponentSequence();
                continue;
                break;
            default:
                comp = new ComponentClass();
                ((ComponentClass*) comp)->re_part[0] = re[ptr];
                ((ComponentClass*) comp)->re_part[1] = '\0';
                ((ComponentClass*) comp)->insert(re[ptr]);
                ptr++;
                break;
        }

        if(is_repetition(re[ptr])){
            int lb, ub;
            switch (re[ptr]){
                case PLUS:
                    ptr++;
                    lb = 1;
                    ub = _INFINITY;
                    break;
                case STAR:
                    ptr++;
                    lb = 0;
                    ub = _INFINITY;
                    break;
                case OPT:
                    ptr++;
                    lb = 0;
                    ub = 1;
                    break;
                case OPEN_QBRACKET:
                    process_quantifier(re, ptr, &lb, &ub);
                    break;
                default:
                    lb = ub = 0;
                    fatal((char*)"parse: wrong repetition char.");
                    break;
            }
            comp = new ComponentRepeat(comp, lb, ub);
        }

        compSeq->add(comp);
    }
    if(re[ptr] != ')') fatal((char*)"parse_sub: sematic wrong.");
    ptr++;

    if(compAlter != nullptr)
    {
        compAlter->add(compSeq);
        return compAlter;
    }
    else return compSeq;
}

Component* parse(char *re) {
    int ptr = 0;
    bool flag_anchor = false;
    auto* compSeq = new ComponentSequence();
    Component* comp = nullptr;
    ComponentAlternation* compAlter = nullptr;

    if(re[ptr] == '^'){
        flag_anchor = true;
        ptr++;
    }

    while(ptr < strlen(re)){
        switch(re[ptr]){
            case '\\':
                comp = parse_escape(ptr, re);
                break;
            case '.':
                ptr++;
                comp = new ComponentClass();
                sprintf(((ComponentClass*)comp)->re_part, ".");
                ((ComponentClass*) comp)->negate();
                break;
            case '[':
                comp = parse_range(ptr, re);
                break;
            case '(':
                comp = parse_subcomp(ptr, re);
                break;
            case '|':
                ptr++;
                if(compAlter == nullptr) compAlter = new ComponentAlternation();
                compAlter->add(compSeq);
                compSeq = new ComponentSequence();
                continue;
                break;
            default:
                comp = new ComponentClass();
                ((ComponentClass*) comp)->re_part[0] = re[ptr];
                ((ComponentClass*) comp)->re_part[1] = '\0';
                ((ComponentClass*) comp)->insert(re[ptr]);
                ptr++;
                break;
        }

        if(is_repetition(re[ptr])){
            int lb = 0, ub = 0;
            switch (re[ptr]){
                case PLUS:
                    ptr++;
                    lb = 1;
                    ub = _INFINITY;
                    break;
                case STAR:
                    ptr++;
                    lb = 0;
                    ub = _INFINITY;
                    break;
                case OPT:
                    ptr++;
                    lb = 0;
                    ub = 1;
                    break;
                case OPEN_QBRACKET:
                    process_quantifier(re, ptr, &lb, &ub);
                    break;
                default:
                    lb = ub = 0;
                    fatal((char*)"parse: wrong repetition char.");
                    break;
            }
            comp = new ComponentRepeat(comp, lb, ub);
        }

        compSeq->add(comp);
    }

    if(compAlter != nullptr)
    {
        compAlter->add(compSeq);
        return compAlter;
    }
    else return compSeq;
}
