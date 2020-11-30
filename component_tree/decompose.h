//
// Created by 钟金诚 on 2020/8/13.
//

#ifndef BECCHI_REGEX_DECOMPOSE_H
#define BECCHI_REGEX_DECOMPOSE_H
#include "parse_component.h"
#include <list>

/*判定decompose提取前缀中是否包含 dotstar-like re part*/
/*
 * 将一条规则分解为不发生（或有限）规则膨胀的规则前缀(R_pre)和规则后缀(R_post)
 * return: R_pre's p_match
 * */
double decompose(char* re, char* R_pre, char* R_post, int threshold = DEAFAULT_THRESHOLD, bool use_pmatch = true, bool control_top = false);

/*
* 分解规则中匹配概率最低的长度确定的子部分
* 注意：同时避免状态膨胀
* return p_match of R_mid
* */
double extract(char *re, char *R_pre, char* R_middle, char *R_post, int &depth, int threshold = 50);

/*
 * 分解规则，尝试提取最简单的部分 (连续的字符和小charclass)
 * return p_match of R_mid
 * */
double extract_simplest(char *re, char *R_pre, char* R_middle, char *R_post);

/*
 * reverse re, make new re match the reverse sequence
 * */
void reverse_re(char* re);

bool is_exactString(char *re);

bool contain_dotstar(char *re);
#endif //BECCHI_REGEX_DECOMPOSE_H
