//
// Created by dy on 6/29/20.
//

#include "Data_Pack.h"

void Data_Pack::init(long long max_len)
{
    this->max_len = max_len;
}


void Data_Pack::destroy()
{

}


bool Data_Pack::put_data(uint8_t *data_buf, long long len, int cnt)
{
    if (len > this->max_len)
        return false;
    this->buf_len = len;
    memcpy(this->buf, data_buf, this->buf_len);
    this->pack_cnt = cnt;
    return true;
}


void Data_Pack::get_data(uint8_t *data_buf, long long &len, int &cnt)
{
    len = this->buf_len;
    memcpy(data_buf, this->buf, len);
    cnt = this->pack_cnt;
}