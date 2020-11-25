//
// Created by 钟金诚 on 2020/11/25.
//

#ifndef BECCHI_REGEX_RUNTIME_FPGA_H
#define BECCHI_REGEX_RUNTIME_FPGA_H

#include <cstdint>

struct _prefix_match{
    uint32_t packet_id;
    uint16_t offset;
    uint16_t DFA_id; //BRAM_id
    uint8_t state;
};

typedef struct _prefix_match prefix_match;

#endif //BECCHI_REGEX_RUNTIME_FPGA_H
