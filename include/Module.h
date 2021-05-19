//
// Created by dy on 6/30/20.
//

#ifndef MY_CLIENT_LINUX_MODULE_H
#define MY_CLIENT_LINUX_MODULE_H
#include "common.h"
#include "FIFO.h"

class Module
{
public:
    FIFO ** input;
    FIFO ** output;
    int input_cnt;
    int output_cnt;
    int packet_cnt;
    uint8_t * data_in;
    uint8_t * data_out;
public:
    void init(FIFO ** &input, FIFO ** &output, int input_cnt, int output_cnt);
    void loop();
};

#endif //MY_CLIENT_LINUX_MODULE_H
