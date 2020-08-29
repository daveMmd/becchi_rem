//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_PARSE_COMPONENT_H
#define BECCHI_REGEX_PARSE_COMPONENT_H

#include "Component.h"
#include "ComponentAlternation.h"
#include "ComponentRepeat.h"
#include "ComponentSequence.h"
#include "ComponentClass.h"

Component* parse(char *re);

#endif //BECCHI_REGEX_PARSE_COMPONENT_H
