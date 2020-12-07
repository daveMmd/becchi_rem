//
// Created by 钟金诚 on 2020/12/6.
//

#ifndef BECCHI_REGEX_PARSE_PCAP_H
#define BECCHI_REGEX_PARSE_PCAP_H

#include<netinet/in.h>


typedef int32_t bpf_int32;
typedef u_int32_t bpf_u_int32;
typedef u_int16_t  u_short;
typedef u_int32_t u_int32;
typedef u_int16_t u_int16;
typedef u_int8_t u_int8;


//pacp文件头结构体

struct pcap_file_header
{
    bpf_u_int32 magic;       /* 0xa1b2c3d4 */
    u_short version_major;   /* magjor Version 2 */
    u_short version_minor;   /* magjor Version 4 */
    bpf_int32 thiszone;      /* gmt to local correction */
    bpf_u_int32 sigfigs;     /* accuracy of timestamps */
    bpf_u_int32 snaplen;     /* max length saved portion of each pkt */
    bpf_u_int32 linktype;    /* data link type (LINKTYPE_*) */
};

//时间戳
struct time_val
{
    int tv_sec;         /* seconds 含义同 time_t 对象的值 */
    int tv_usec;        /* and microseconds */
};

//pcap数据包头结构体
struct pcap_pkthdr
{
    struct time_val ts;  /* time stamp */
    bpf_u_int32 caplen; /* length of portion present */
    bpf_u_int32 len;    /* length this packet (off wire) */
};

#endif //BECCHI_REGEX_PARSE_PCAP_H
