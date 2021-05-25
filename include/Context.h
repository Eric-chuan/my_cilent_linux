//
// Created by xj on 12/10/20.
//

#ifndef MY_CLIENT_LINUX_CONTEXT_H
#define MY_CLIENT_LINUX_CONTEXT_H
#include "common.h"
#include <atomic>

class Context
{
public:
    int packet_cnt;
    int *client_echo;
    char **input_URL;
    bool start_delivery;
    bool local_output;
    int set_latlevel;
    std::atomic<int> centerIdx;
    std::atomic<int> pulled_centerIdx;
public:
    void init(int packet_cnt,  int c);
};

#endif //MY_CLIENT_LINUX_CONTEXT_H
