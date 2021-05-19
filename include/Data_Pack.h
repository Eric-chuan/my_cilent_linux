//
// Created by dy on 6/29/20.
//

#ifndef MY_CLIENT_LINUX_DATA_PACK_H
#define MY_CLIENT_LINUX_DATA_PACK_H
#include "common.h"

class Data_Pack
{
public:
    long long max_len;
    long long buf_len;
    uint8_t * buf;
    int pack_cnt;

public:
    void init(long long max_len);
    void destroy();
    bool put_data(uint8_t * data_buf, long long len, int cnt);
    void get_data(uint8_t * data_buf, long long &len, int &cnt);
};

#endif //MY_CLIENT_LINUX_DATA_PACK_H
