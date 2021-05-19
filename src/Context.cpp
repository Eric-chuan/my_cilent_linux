//
// Created by xj on 12/10/20.
//

#include "Context.h"

void Context::init(int packet_cnt, int *client_echo)
{
    this->packet_cnt = packet_cnt;
    this->client_echo = client_echo;
    //memset(selection, 0, this->num_of_streams);
    this->start_delivery = false;
    this->local_output = false;
}
