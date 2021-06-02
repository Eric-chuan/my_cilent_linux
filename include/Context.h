//
// Created by xj on 12/10/20.
//

#ifndef MY_CLIENT_LINUX_CONTEXT_H
#define MY_CLIENT_LINUX_CONTEXT_H
#include "common.h"
#include <atomic>
#define VIEW_NUM 25  //all view number
#define TILE_LEFT 12 //4H 8L
#define TILE_RIGHT 12  //4H 8L
#define TILE_NUM 25   //1 CENTER + 4 LEFT + 4 RIGHT
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
