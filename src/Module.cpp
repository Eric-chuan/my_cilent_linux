//
// Created by dy on 6/30/20.
//

#include "Module.h"

void Module::init(FIFO ** &input, FIFO ** &output, int input_cnt, int output_cnt)
{
    this->input_cnt = input_cnt;
    this->output_cnt = output_cnt;
    this->input = input;
    this->output = output;
    this->packet_cnt = packet_cnt;
    this->data_in = new uint8_t[MAX_FIFO_SIZE];
    this->data_out = new uint8_t[MAX_FIFO_SIZE];
}


void Module::loop()
{

}
