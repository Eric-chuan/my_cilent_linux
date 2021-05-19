#include "Pull_Stream.h"
#include "NvDecoder.h"
#include <thread>
#include <unistd.h>
//#include "SDL2/SDL.h"


using namespace std;

extern float get_time_diff_ms(timeval start, timeval end)
{
    long time_ms_end =  (end.tv_sec * 1000000 + end.tv_usec);
    long time_ms_start =  (start.tv_sec * 1000000 + start.tv_usec);
    return float(time_ms_end - time_ms_start) / 1000;
}

extern std::atomic<int> sys_f_cnt;
std::atomic<int> sys_f_cnt;
int main(int argc,char *argv[])
{
    cuInit(0);
    CUdevice nvdevice;
    cuDeviceGet(&nvdevice, 0);
    CUcontext cuContext = NULL;
    cuCtxCreate(&cuContext, CU_CTX_SCHED_SPIN, 0);

    sys_f_cnt = 0;
    int c;
    int option_index = 0;
    int fifo_len = 5;
    int frame_num = 100;
    int start_id = 10;
    char * inputurl = "../ts.ts";

    int fifo_num = 1 + 1;
    long long fifo_data_size = fifo_len * MAX_FIFO_SIZE;
    uint8_t * fifo_data = new uint8_t[fifo_data_size * fifo_num];


    FIFO **video_stream_pulled_265 = new FIFO * [1];
    FIFO *video_stream_decoded_yuv = new FIFO();

    Stream_Puller **stream_puller = new Stream_Puller * [1];
    NvDecoder **decoder = new NvDecoder * [1];

    int module_num = 0;
    module_num += stream_puller[0]->get_mem_cnt();
    module_num += decoder[0]->get_mem_cnt();
    uint8_t * module_data = new uint8_t[module_num * MAX_FIFO_SIZE];

    for(int i = 0; i < 1; i++){
        video_stream_pulled_265[i] = new FIFO();
        video_stream_pulled_265[i]->init(fifo_len, MAX_FIFO_SIZE, &fifo_data[i * fifo_data_size]);
    }
    video_stream_decoded_yuv->init(fifo_len, MAX_FIFO_SIZE, &fifo_data[1 * fifo_data_size]);


    FIFO ** stream_puller_output = new FIFO * [2];
    FIFO ** decoder_input = new FIFO * [1];
    for(int i = 0; i < 1; i++){
        stream_puller_output[i] = video_stream_pulled_265[i];
        decoder_input[i] = video_stream_pulled_265[i];
    }
    FIFO ** decoder_output = new FIFO * [1];
    decoder_output[0] = video_stream_decoded_yuv;

    /*SDL_Window * window;
    SDL_Renderer *renderer;
    SDL_RendererInfo info;
    SDL_Rect    rect;
    SDL_Texture *texture;

    window = SDL_CreateWindow("Client Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderer, 3840, 2160);
    SDL_GetRendererInfo(renderer, &info);
    fprintf(stderr, "Using %s rendering\n", info.name);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, 3840, 2160);

    rect.x = 0;
    rect.y = 0;
    rect.w = 3840;
    rect.h = 2160;
    SDL_Event evt;*/

    for(int i = 0; i < 1; i++){
        stream_puller[i] = new Stream_Puller();
        int sp_buf_index =i * MAX_FIFO_SIZE * stream_puller[i]->get_mem_cnt();
        stream_puller[i]->set_buf(&module_data[sp_buf_index]);
        decoder[i] = new NvDecoder();
        int dec_buf_index = 1 * MAX_FIFO_SIZE * stream_puller[0]->get_mem_cnt() + i * MAX_FIFO_SIZE * stream_puller[i]->get_mem_cnt();
        decoder[i]->set_buf(&module_data[dec_buf_index]);
        stream_puller[i]->init(NULL, stream_puller_output + i, 1, 1, inputurl);
        //use cude_device to choose graphic cards. Set i % 2 to balance loads
        //here set to 0 to use the first card
        decoder[i]->init(decoder_input + i, decoder_output, 1, 1, 0, &cuContext);
    }

    timeval t_0;
    gettimeofday(&t_0, NULL);



    thread *stream_puller_thread = new thread[1];
    thread *decoder_send_thread = new thread[1] ;
    thread *decoder_receive_thread = new thread[1];

    for (int i = 0; i < 1; i++){
        stream_puller_thread[i] = std::thread(&Stream_Puller::loop, std::ref(stream_puller[i]));
        decoder_send_thread[i] = std::thread(&NvDecoder::loop_nvdecoder_send, std::ref(decoder[i]));
        decoder_receive_thread[i] = std::thread(&NvDecoder::loop_nvdecoder_receive, std::ref(decoder[i]));
    }

    for(int i = 0; i < 1; i++){
        stream_puller_thread[i].detach();
        decoder_send_thread[i].detach();
        decoder_receive_thread[i].detach();
    }

    fprintf(stderr, "Waiting for stream.\n");

    while (!decoder[0]->stop) {
        usleep(10000);
    }

    sys_f_cnt = decoder[0]->fcnt_in;

    fprintf(stderr, "Totally %d frames processed.\n", (int)sys_f_cnt);
    FILE * fp = fopen("../../log_client.txt","w");
    fprintf(fp, "get_packet_time, decode_time, display_time\n");
    for (int i = 0 ; i < sys_f_cnt ; i ++) {
        fprintf(fp, "%ld,%ld %ld,%ld %ld,%ld\n",
                video_stream_pulled_265[0]->pushed_time[i].tv_sec, video_stream_pulled_265[0]->pushed_time[i].tv_usec,
                video_stream_decoded_yuv->pushed_time[i].tv_sec, video_stream_decoded_yuv->pushed_time[i].tv_usec,
                video_stream_decoded_yuv->popped_time[i].tv_sec, video_stream_decoded_yuv->popped_time[i].tv_usec);
    }
    fclose(fp);
    //SDL_DestroyTexture(texture);
    //SDL_DestroyRenderer(renderer);
    //SDL_DestroyWindow(window);
    //texture = NULL;
    //window = NULL;
    //renderer = NULL;
    //SDL_Quit();

    for(int i = 0; i < 1; i++){
        video_stream_pulled_265[i]->destroy();
    }
    video_stream_decoded_yuv->destroy();


    for(int i = 0; i < 1; i++){
        delete stream_puller[i];
        delete decoder[i];
    }
    delete [] stream_puller;
    delete [] decoder;


    delete [] stream_puller_output;
    delete [] decoder_input;
    delete [] decoder_output;


    for(int i = 0; i < 1; i++){
        delete video_stream_pulled_265[i];
    }
    delete [] video_stream_pulled_265;

    delete video_stream_decoded_yuv;

    delete [] fifo_data;
    delete [] module_data;
    delete [] stream_puller_thread;
    delete [] decoder_send_thread;
    delete [] decoder_receive_thread;
    return 0;
}
