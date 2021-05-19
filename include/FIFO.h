//
// Created by dy on 6/29/20.
//

#ifndef MY_CLIENT_LINUX_FIFO_H
#define MY_CLIENT_LINUX_FIFO_H
#include "common.h"
#include "Data_Pack.h"
#include <mutex>
#include <atomic>

extern float get_time_diff_ms(timeval start, timeval end);

class FIFO
{
public:
    int fifo_size;
    int max_data_pack_size;
    uint8_t * data;
    Data_Pack ** buffer;
    std::atomic<int> head;
    std::atomic<int> tail;
    int pack_cnt; // number of data_pack
    timeval * pushed_time;
    timeval * popped_time;
    float * mcp_time_in;
    float * mcp_time_out;
    timeval * cpin_endtime;
    timeval * cpout_starttime;
    int p_pushed_time, p_popped_time;
private:
    std::atomic<bool> mut;
    int processed_head;
    int processed_tail;
    std::atomic<bool> * in_process;
private:
    void add_one(std::atomic<int> &ptr);
    bool is_internal_empty();
    bool is_internal_full();
public:
    void init(int fifo_size, int max_data_pack_size, uint8_t * data_ptr);
    bool is_empty();
    bool is_full();
    void destroy();
    uint8_t * fetch_put_pointer(long long len, int cnt, int &ptr_num);
    uint8_t * fetch_get_pointer(long long &len, int &cnt, int &ptr_num);
    void release_put_pointer(int ptr_num);
    void release_get_pointer(int ptr_num);
    bool put(uint8_t *data_buf, long long len, int cnt, bool output = false, int timeout_in_ms = 0);
    bool get(uint8_t *data_buf, long long &len, int &cnt, bool output = false, int timeout_in_ms = 0);
};

#endif //MY_CLIENT_LINUX_FIFO_H
