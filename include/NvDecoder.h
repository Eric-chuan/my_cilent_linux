//
// Created by xj on 10/11/20.
//

#ifndef MY_CLIENT_LINUX_NVDECODER_H
#define MY_CLIENT_LINUX_NVDECODER_H
#include "common.h"
#include "Module.h"
#include "Context.h"

//xujun
/*#include "dynlink_cuviddec.h" // <cuviddec.h>
#include "dynlink_nvcuvid.h"  // <nvcuvid.h>
#include "dynlink_cuda.h"    // <cuda.h>
#include "helper_cuda_drvapi.h"
#include "FrameQueue.h"*/
#include <cuda.h>
#include "nvcuvid.h"
#include <memory>
#include <atomic>

#include <fcntl.h>
#include <stdio.h>
//dy
#include <cvt.cuh>

#include "FramePresenterGLX.h"

#define VIEW_NUM 25  //all view number
#define TILE_LEFT 12 //4H 8L
#define TILE_RIGHT 12  //4H 8L
#define TILE_NUM 25   //1 CENTER + 4 LEFT + 4 RIGHT

#define MAX_DEC_SURF 20

extern float get_time_diff_ms(timeval start, timeval end);
static std::atomic<int> cnt_out;
static std::atomic<bool> is_decoding[MAX_DEC_SURF];

class videoparser
{
private:
    static int HandleVideoSequence(void *pUserData, CUVIDEOFORMAT *pFormat);
    static int HandlePictureDecode(void *pUserData, CUVIDPICPARAMS *pPicParams);
    static int HandlePictureDisplay(void *pUserData, CUVIDPARSERDISPINFO *pPicParams);

public:
    CUvideoparser nvparser_;
    void init(void *pNvDecoder);
};

class NvDecoder : public Module
{
private:
    int cnt_in;
    std::atomic<bool> sender_stop;
    uint8_t * data_buf;
    int nvDev_id;
    Context *sys_ctx;
public:
    CUvideodecoder nvdecoder;
    videoparser nvparser;
    //FrameQueue *nvqueue;
    CUcontext nvcontext;
    int fcnt_in, fcnt_out;
    std::atomic<bool> stop;
    void init(FIFO ** input, FIFO ** output, int input_cnt, int output_cnt,int cuda_device, CUcontext * ctx,  Context *sys_ctx);
    void loop_nvdecoder_send();
    void loop_nvdecoder_receive();
    int viewIdx_selector(int c, int shift);
    int get_mem_cnt(){
        return 1;
    };
    void set_buf(uint8_t * buf) {
        this->data_buf = buf;
    };
};

#endif //MY_CLIENT_LINUX_NVDECODER_H

