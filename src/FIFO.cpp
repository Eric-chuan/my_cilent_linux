//
// Created by dy on 6/29/20.
//

#include "FIFO.h"

void FIFO::add_one(std::atomic<int> &ptr)
{
    ptr = (ptr + 1) % this->fifo_size;
}


void FIFO::init(int fifo_size, int max_data_pack_size, uint8_t * data_ptr)
{
    this->fifo_size = fifo_size;
    this->max_data_pack_size = max_data_pack_size;
    this->data = data_ptr;
    this->buffer = new Data_Pack * [this->fifo_size];
    for(int p_id = 0 ; p_id < this->fifo_size ; p_id ++) {
        this->buffer[p_id] = new Data_Pack();
        this->buffer[p_id]->init(this->max_data_pack_size);
        this->buffer[p_id]->buf = &this->data[p_id * this->max_data_pack_size];
    }
    this->pushed_time = new timeval[MAX_LOG_CNT];
    this->popped_time = new timeval[MAX_LOG_CNT];
    this->mcp_time_in = new float[MAX_LOG_CNT];
    this->mcp_time_out = new float[MAX_LOG_CNT];
    this->cpin_endtime = new timeval[MAX_LOG_CNT];
    this->cpout_starttime = new timeval[MAX_LOG_CNT];

    this->p_pushed_time = 0;
    this->p_popped_time = 0;

    this->pack_cnt = 0;
    this->head = 0;
    this->tail = 0;
    this->processed_head = 0;
    this->processed_tail = 0;
    this->in_process = new std::atomic<bool>[this->fifo_size];
    for(int p_id = 0 ; p_id < this->fifo_size ; p_id ++) {
        this->in_process[p_id] = false;
    }
    this->mut = false;
}

bool FIFO::is_internal_empty()
{
    return this->is_empty() && (this->processed_tail == this->tail);
}

bool FIFO::is_internal_full()
{
    return this->is_full() && (this->processed_head == this->head);
}

bool FIFO::is_empty()
{
    return this->pack_cnt == 0;
}


bool FIFO::is_full()
{
    return this->pack_cnt == this->fifo_size;
}


void FIFO::destroy()
{
    if (this->pack_cnt > 0)
        fprintf(stderr, "WARNING: FIFO not empty!\n");
    for(int p_id = 0 ; p_id < this->fifo_size ; p_id ++) {
        this->buffer[p_id]->destroy();
        delete this->buffer[p_id];
    }
    this->data = NULL;
    delete [] this->buffer;

    delete [] this->pushed_time;
    delete [] this->popped_time;
    delete [] this->in_process;
    delete [] this->mcp_time_in;
    delete [] this->mcp_time_out;
    delete [] this->cpin_endtime;
    delete [] this->cpout_starttime;
}
uint8_t * FIFO::fetch_put_pointer(long long len, int cnt, int &ptr_num)
{
    gettimeofday(&this->pushed_time[p_pushed_time], NULL);
    int sleep_cnt = 0;
    while (true) {
        if (this->is_full()) {
            usleep(1000);
            sleep_cnt ++;
            continue;
        }
        if (!this->mut) {
            this->mut = true;
            if(this->is_full()) {
                this->mut = false;
                usleep(1000);
                sleep_cnt ++;
                continue;
            } else {
                break;
            }
        }
    }
    ptr_num = this->tail;
    int cntt = 0;
    while(this->in_process[ptr_num]) {
        usleep(100);
        cntt++;
    }
    this->in_process[ptr_num] = true;
    this->add_one(this->tail);
    this->pack_cnt ++;
    this->mut = false;
    this->buffer[ptr_num]->buf_len = len;
    this->buffer[ptr_num]->pack_cnt = cnt;
    return this->buffer[ptr_num]->buf;
}
uint8_t * FIFO::fetch_get_pointer(long long &len, int &cnt, int &ptr_num)
{
    int sleep_cnt = 0;
    while (true) {
        if (this->is_empty()) {
            sleep_cnt ++;
            usleep(1000);
            continue;
        }
        if (!this->mut) {
            this->mut = true;
            if(this->is_empty()) {
                this->mut = false;;
                sleep_cnt ++;
                usleep(1000);
                continue;
            } else {
                break;
            }
        }
    }
    ptr_num = this->head;
    int cntt = 0;
    while(this->in_process[ptr_num]) {
        cntt++;
        usleep(100);
    }
    this->in_process[ptr_num] = true;
    this->add_one(this->head);
    this->pack_cnt --;
    this->mut = false;
    len = this->buffer[ptr_num]->buf_len;
    cnt = this->buffer[ptr_num]->pack_cnt;
    gettimeofday(&this->popped_time[p_popped_time++], NULL);
    return this->buffer[ptr_num]->buf;
}
void FIFO::release_put_pointer(int ptr_num)
{
    this->in_process[ptr_num] = false;
    gettimeofday(&this->pushed_time[p_pushed_time++], NULL);
}

void FIFO::release_get_pointer(int ptr_num)
{
    this->in_process[ptr_num] = false;
}

bool FIFO::put(uint8_t *data_buf, long long len, int cnt, bool output, int  timeout_in_ms)
{
    gettimeofday(&this->pushed_time[p_pushed_time], NULL);
    timeval t1, t2;
    int sleep_cnt = 0;
    while (true) {
        if (this->is_full()) {
            usleep(1000);
            sleep_cnt ++;
            if(timeout_in_ms > 0 && sleep_cnt >= timeout_in_ms) return false;
            continue;
        }
        if (!this->mut) {
            this->mut = true;
            if(this->is_full()) {
                this->mut = false;
                usleep(1000);
                sleep_cnt ++;
                if(timeout_in_ms > 0 && sleep_cnt >= timeout_in_ms) return false;
                continue;
            } else {
                break;
            }
        }
    }
    int ptr = this->tail;
    int cntt = 0;
    while(this->in_process[ptr]) {
        usleep(100);
        cntt++;
    }
    this->in_process[ptr] = true;
    this->add_one(this->tail);
    this->pack_cnt ++;
    this->mut = false;
    gettimeofday(&t1, NULL);
    this->buffer[ptr]->put_data(data_buf, len, cnt);
    gettimeofday(&t2, NULL);
    this->mcp_time_in[p_pushed_time] = get_time_diff_ms(t1, t2);
    gettimeofday(&this->cpin_endtime[p_pushed_time++], NULL);
    this->in_process[ptr] = false;
    return true;
}


bool FIFO::get(uint8_t *data_buf, long long &len, int &cnt, bool output, int timeout_in_ms)
{
    int sleep_cnt = 0;
    timeval t1, t2;
    while (true) {
        if (this->is_empty()) {
            sleep_cnt ++;
            usleep(1000);
            if(timeout_in_ms > 0 && sleep_cnt >= timeout_in_ms) return false;
            continue;
        }
        if (!this->mut) {
            this->mut = true;
            if(this->is_empty()) {
                this->mut = false;;
                sleep_cnt ++;
                usleep(1000);
                if(timeout_in_ms > 0 && sleep_cnt >= timeout_in_ms) return false;
                continue;
            } else {
                break;
            }
        }
    }
    int ptr = this->head;
    int cntt = 0;
    while(this->in_process[ptr]) {
        cntt++;
        usleep(100);
    }
    this->in_process[ptr] = true;
    this->add_one(this->head);
    this->pack_cnt --;
    this->mut = false;
    gettimeofday(&this->cpout_starttime[p_popped_time], NULL);
    gettimeofday(&t1, NULL);
    this->buffer[ptr]->get_data(data_buf, len, cnt);
    gettimeofday(&t2, NULL);
    this->mcp_time_out[p_popped_time] = get_time_diff_ms(t1, t2);
    this->in_process[ptr] = false;
    gettimeofday(&this->popped_time[p_popped_time++], NULL);
    return true;
}
