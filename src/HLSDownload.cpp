//
// Created by hjc on 5/24/21.
//
#include "HLSDownload.h"

#define re_time 40000

extern std::atomic<int> sys_f_cnt;
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct*)userp;
    if (mem->reserved == 0)
    {
        CURLcode res;
        double filesize = 0.0;

        res = curl_easy_getinfo(mem->c, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
        if((CURLE_OK == res) && (filesize>0.0))
        {
            mem->memory = (char*)realloc(mem->memory, (int)filesize + 2);
            if (mem->memory == NULL) {
                return 0;
            }
            mem->reserved = (int)filesize + 1;
        }
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

void HLSDownload::init(FIFO ** input, FIFO ** output, int input_cnt, int output_cnt,char *input_url, Context *ctx)
{
    this->input_cnt = input_cnt;
    this->input = input;
    this->output_cnt = output_cnt;
    this->output = output;
    this->master_url = input_url;
    this->ctx = ctx;
    this->packet_cnt = ctx->packet_cnt;

    this->segment_fifo = new FIFO();
    uint8_t *segment_fifo_data = new uint8_t[4 * MAX_SEG_SIZE];
    segment_fifo->init(4, MAX_SEG_SIZE, segment_fifo_data);

    curl_global_init(CURL_GLOBAL_ALL);
    this->master_playlist = new HLSMasterPlaylist();
    memset(master_playlist, 0x00, sizeof(HLSMasterPlaylist));
    this->master_playlist->url = master_url;
    size_t size = 0;
    get_hls_data_from_url(this->master_url, &(this->master_playlist->source), &size, STRING);
    init_media_playlists();
}

int HLSDownload::init_media_playlists()
{
    int me_index = 0;
    bool url_expected = false;
    char *src = this->master_playlist->source;
    while(*src != '\0'){
        char *end_ptr = strchr(src, '\n');
        if (!end_ptr) {
            break;
        }
        *end_ptr = '\0';
        if (*src == '#') {
            url_expected = false;
            if (!strncmp(src, "#EXT-X-STREAM-INF:", 18)) {
                url_expected = true;
            }
        } else if (url_expected) {
            size_t len = strlen(src);
            // here we will fill new playlist
            this->media_playlists[me_index] = (HLSMediaPlaylist*)malloc(sizeof(HLSMediaPlaylist));
            memset(this->media_playlists[me_index], 0x00, sizeof(HLSMediaPlaylist));
            size_t max_length = len + strlen(this->master_url) + 10;
            char* extend_url = (char*)malloc(max_length);
            sprintf(extend_url, "%s/../%s", this->master_url, src);
            this->media_playlists[me_index]->url = extend_url;
            me_index++;
            url_expected = false;
        }
        src = end_ptr + 1;
    }
    this->stream_num = me_index;
    return 0;
}
int HLSDownload::update_m3u8()
{
    size_t size = 0;
    for (int i = 0; i < stream_num; i++) {
        get_hls_data_from_url(this->media_playlists[i]->url, &this->media_playlists[i]->source, &size, STRING);
        bool url_expected = false;
        int seg_index = 0;
        char *src = this->media_playlists[i]->source;
        while(*src != '\0'){
            char *end_ptr = strchr(src, '\n');
            if (!end_ptr) {
                break;
            }
            *end_ptr = '\0';
            if (*src == '#') {
                url_expected = false;
                if (!strncmp(src, "#EXTINF:", 8)) {
                    url_expected = true;
                }
            } else if (url_expected) {
                size_t len = strlen(src);
                this->media_playlists[i]->media_segments[seg_index] = (HLSMediaSegment*)malloc(sizeof(HLSMediaSegment));
                memset(this->media_playlists[i]->media_segments[seg_index], 0x00, sizeof(HLSMediaSegment));
                // here we will fill new playlist
                size_t max_length = len + strlen(this->media_playlists[i]->url) + 10;
                char* extend_url = (char*)malloc(max_length);
                char* sub_url = strstr(this->media_playlists[i]->url, "master");
                char* head_url = (char*)malloc(sub_url - this->media_playlists[i]->url);
                memcpy(head_url, this->media_playlists[i]->url, sub_url -this->media_playlists[i]->url);
                sprintf(extend_url, "%s%s/../%s", head_url, sub_url + 15, src);
                this->media_playlists[i]->media_segments[seg_index]->url = extend_url;
                seg_index++;
                url_expected = false;
            }
            src = end_ptr + 1;
        }
        this->segment_num = seg_index;
    }
    return 0;
}
long HLSDownload::get_hls_data_from_url(char* url, char** data, size_t *size, int type)
{
    CURL *c = curl_easy_init();
    CURLcode res;
    long http_code = 0;
    MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.memory[0] = '\0';
    chunk.size = 0;
    chunk.reserved = 0;
    chunk.c = c;
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&chunk);
    if (type == BINARY) {
        curl_easy_setopt(c, CURLOPT_LOW_SPEED_LIMIT, 2L);
        curl_easy_setopt(c, CURLOPT_LOW_SPEED_TIME, 3L);
    }
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    res = curl_easy_perform(c);
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &http_code);
    if (type == STRING) {
        *data = strdup(chunk.memory);
    } else if (type == BINARY) {
        *data = (char*)malloc(chunk.size);
        if (chunk.size > 16 && (chunk.memory[0] == 0x89 && chunk.memory[1] == 0x50 && chunk.memory[2] == 0x4E)) {
            *data = (char*)memcpy(*data, chunk.memory + 16, chunk.size);
        } else {
            *data = (char*)memcpy(*data, chunk.memory, chunk.size);
        }
    }
    *size = chunk.size;
    if (chunk.memory) {
        free(chunk.memory);
    }
    curl_easy_cleanup(c);
    return http_code;
}
void HLSDownload::vod_download_segment(int centerIdx, int segIdx)
{
    //FILE *out_file = fopen("../out.ts", "wb");
    update_m3u8();
    HLSMediaSegment* ms = this->media_playlists[centerIdx]->media_segments[0];
    int tries = 20;
    long http_code = 0;
    while(tries) {
        http_code = get_hls_data_from_url(ms->url, (char**)&(ms->data), (size_t*)&(ms->data_len), BINARY);
        if (200 != http_code || ms->data_len == 0) {
            --tries;
            usleep(1000);
            continue;
        }
        break;
    }
    //fwrite(ms->data, ms->data_len, sizeof(uint8_t), out_file);
    this->segment_ts_len = ms->data_len;
    this->segment_ts_data = (uint8_t*)malloc(ms->data_len * sizeof(uint8_t));
    memcpy(this->segment_ts_data, ms->data, ms->data_len);
    free((ms->data));
    this->ctx->pulled_centerIdx = centerIdx;
    printf("pulled view %d\n", centerIdx);
    //fclose(out_file);
}

void HLSDownload::loop_recv()
{
    int segIdx = 0;
    int scnt = 0;
    while(true) {
        vod_download_segment(this->ctx->centerIdx, segIdx);
        this->segment_fifo->put(this->segment_ts_data, this->segment_ts_len, scnt);
        free(this->segment_ts_data);
        scnt++;
        usleep(900000);
        if (this->packet_cnt > 0 && scnt >= this->packet_cnt){
            usleep(30000);
            break;
        }
    }
}

void HLSDownload::loop()
{
    unsigned int ts_position = 0;
    unsigned int received_length = 0;
    unsigned int available_length = 0;
    unsigned int max_ts_length = 65536;
    unsigned int es_packet_count = 0;
    bool start_flag = 0;
    bool received_flag = 0;
    uint8_t* payload_position = NULL;
    //uint8_t* this->data_buf = (uint8_t*)malloc(max_ts_length);
    int buffer_len = 191760;//188*204*5
    TSHeader* ts_header;
    PESInfo* pes;
    //FILE* es_fp = fopen("../demo.hevc", "wb");
    int packet_length = 188;
    int cnt = 0;
    int segIdx = 0;
    uint8_t *segment_ts_data_recv = new uint8_t[MAX_SEG_SIZE];
    long long segment_ts_len_recv;
    int scnt_recv = 0;
    while(true) {
        this->segment_fifo->get(segment_ts_data_recv, segment_ts_len_recv, scnt_recv);
        uint8_t* current_p = segment_ts_data_recv;
        unsigned int read_size = (segment_ts_len_recv >= buffer_len) ? buffer_len : segment_ts_len_recv;
        uint8_t* buffer_data = segment_ts_data_recv;
        while(current_p - segment_ts_data_recv < segment_ts_len_recv){
            while(current_p < buffer_data + read_size){
                ts_header = new TSHeader(current_p);
                if(ts_header->PID == 256){
                    es_packet_count++;
                    if(ts_header->adapation_field_control == 1){
                        payload_position = current_p + 4;
                    }else if(ts_header->adapation_field_control == 3){
                        payload_position = current_p + 4 + current_p[4] + 1;
                    }
                    if(ts_header->payload_uint_start_indicator != 0){
                        start_flag =1;
                        cnt++;
                    }
                    if(start_flag && payload_position){
                        available_length = packet_length + current_p -payload_position;
                        if(ts_header->payload_uint_start_indicator != 0){
                            if(received_length > 0){
                                pes = new PESInfo(this->data_buf);
                                if(pes->packet_start_code_prefix != 0x000001){
                                    printf("pes is not correct.received %d es packet\n",es_packet_count);
                                    return;
                                }
                                if(pes->PES_packet_data_length != 0){
                                    //fwrite(pes->elementy_stream_position, received_length, 1, es_fp);
                                    this->output[0]->put(pes->elementy_stream_position, received_length, cnt - 1);
                                    usleep(35000);
                                }
                                delete pes;
                                memset(this->data_buf, 0, received_length);
                                received_length = 0;
                            }
                            received_flag = 1;
                        }
                        if(received_flag){
                            if(received_length + available_length > MAX_FIFO_SIZE){
                                // max_ts_length = max_ts_length * 2;
                                // this->data_buf = (uint8_t*)realloc(this->data_buf,max_ts_length);
                                usleep(1000);
                            }
                            memcpy(this->data_buf + received_length, payload_position, available_length);
                            received_length += available_length;
                            if (segment_ts_data_recv + segment_ts_len_recv - current_p == 188) {
                                printf("end at frame %d\n", cnt);
                                pes = new PESInfo(this->data_buf);
                                if(pes->PES_packet_data_length != 0){
                                    //fwrite(pes->elementy_stream_position, received_length, 1, es_fp);
                                    this->output[0]->put(pes->elementy_stream_position, received_length, cnt);
                                    usleep(35000);
                                }
                                memset(this->data_buf, 0, received_length);
                                received_length = 0;
                                delete pes;
                            }
                        }
                    }
                }
            current_p += packet_length;
            delete ts_header;
            }
        read_size = (segment_ts_data_recv - current_p >= buffer_len - segment_ts_len_recv) ? buffer_len : segment_ts_data_recv - current_p + segment_ts_len_recv;
        buffer_data = current_p;
        }

    }
    free(segment_ts_data_recv);
}
