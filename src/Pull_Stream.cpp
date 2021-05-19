//
// Created by xj on 9/26/20.
//
#include "Pull_Stream.h"

#define re_time 40000

extern std::atomic<int> sys_f_cnt;


void Stream_Puller::init(FIFO ** input, FIFO ** output, int input_cnt, int output_cnt,char *input_url)
{
    this->input_cnt = input_cnt;
    this->input = input;
    this->output_cnt = output_cnt;
    this->output = output;
    this->input_url = input_url;
}

void Stream_Puller::loop()
{
    int cnt = 0;
    int in = open(this->input_url, O_RDONLY);
    fcntl(in, F_SETPIPE_SZ, 12582912);
    uint8_t *pack;
    int ts_len = 188;
    int data_len = 0;
    bool has_output = true;
    int adaption_field_control;
    gettimeofday(&tv_init, NULL);
    uint8_t buffer[65536];
    int buffer_ptr = 0;
    bool start_recving = false;
    int sleep_cnt ;
    int my_offset;
    while (true) {
        int read_size;
        sleep_cnt = 0;
        while (true) {
            read_size = read(in, &buffer[buffer_ptr], sizeof(buffer) - buffer_ptr);
            buffer_ptr += read_size;
            if(buffer_ptr >= ts_len) break;
            usleep(1000);
            sleep_cnt ++;
            if (start_recving && sleep_cnt >= 200) break;
        }
        start_recving = true;
        if (buffer_ptr == 0) break;
        int start_ptr = 0;
        while(buffer_ptr - start_ptr >= ts_len) {
            pack = &buffer[start_ptr];
            if (pack[2] == 0x00) {
                cnt ++;
                if (!has_output) {
                    fprintf(stderr, "Special frame %d without adaption_field.\n", cnt - 1); //Will never happen.
                    this->output[0]->put(this->data_buf, data_len - my_offset, cnt);
                }
                has_output = false;
                data_len = 0;
            }
            adaption_field_control = pack[3] >> 4 & 0x03;
            if (pack[2] == 0x11) {
                if (adaption_field_control == 1 && pack[1] == 0x00) {
                    memcpy(&this->data_buf[data_len], &pack[4], 184);
                    data_len += 184;
                }
                if (adaption_field_control == 1 && pack[1] == 0x40) {
                    my_offset = pack[8] * 92;
                    memcpy(&this->data_buf[data_len], &pack[23], 165);
                    data_len += 165;
                }
                if (adaption_field_control == 3) {
                    int offset = pack[4];
                    memcpy(&this->data_buf[data_len], &pack[5 + offset], (188 - (5 + offset)));
                    data_len += (188 - (5 + offset));
                    this->output[0]->put(this->data_buf, data_len - my_offset, cnt);
                    has_output = true;
                }
            }
            start_ptr += ts_len;
        }
        if (buffer_ptr > start_ptr) memcpy(&buffer[0], &buffer[start_ptr], buffer_ptr - start_ptr);
        buffer_ptr -= start_ptr;
    }
    sys_f_cnt = cnt;
    if(!has_output) {
        this->output[0]->put(this->data_buf, data_len - my_offset, cnt);
    }
    close(in);
	fflush(stdout);
}
