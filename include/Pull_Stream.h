//
// Created by xj on 9/26/20.
//

#ifndef MY_CLIENT_LINUX_PULL_STREAM_H
#define MY_CLIENT_LINUX_PULL_STREAM_H
#include "common.h"
#include "Module.h"
#include <atomic>
#include <fcntl.h>

static timeval tv_init;


class Stream_Puller : public Module
{
private:
    char *input_url;
    uint8_t * data_buf;
    //timeval * tout;
public:
    void init(FIFO ** input, FIFO ** output, int input_cnt, int output_cnt, char *input_url);
    void loop();
    int get_mem_cnt(){
        return 1;
    };
    void set_buf(uint8_t * buf) {
        this->data_buf = buf;
    };
};

#endif //MY_CLIENT_LINUX_PULL_STREAM_H

