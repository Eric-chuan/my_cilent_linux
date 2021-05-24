//
// Created by xj on 10/11/20.
//
#include "NvDecoder.h"

#define y_data_len (3840 * 2160)
#define u_data_len (3840 * 2160 / 4)
#define v_data_len (3840 * 2160 / 4)
#define yuv_data_len (3840 * 2160 * 3 / 2)

//sem_t Stream_Puller::init_done;
extern std::atomic<int> sys_f_cnt;

void videoparser::init(void *pNvDecoder)
{
    CUVIDPARSERPARAMS oVideoParserParameters;
    memset(&oVideoParserParameters, 0, sizeof(CUVIDPARSERPARAMS));
    oVideoParserParameters.CodecType = cudaVideoCodec_HEVC;
    oVideoParserParameters.ulMaxNumDecodeSurfaces = 20;
    oVideoParserParameters.ulMaxDisplayDelay = 0;
    oVideoParserParameters.pfnSequenceCallback = HandleVideoSequence;    // Called before decoding frames and/or whenever there is a format change
    oVideoParserParameters.pfnDecodePicture = HandlePictureDecode;    // Called when a picture is ready to be decoded (decode order)
    oVideoParserParameters.pfnDisplayPicture = HandlePictureDisplay;   // Called whenever a picture is ready to be displayed (display order)
    oVideoParserParameters.pUserData = pNvDecoder;
    oVideoParserParameters.ulErrorThreshold = 100;
    CUresult oResult = cuvidCreateVideoParser(&this->nvparser_, &oVideoParserParameters);
}

int videoparser::HandleVideoSequence(void *pUserData, CUVIDEOFORMAT *pFormat)
{
    //printf("HandleVideoSequence of decoder %d.\n", ((NvDecoder *)pUserData)->dec_id);
    return 1;
}

int videoparser::HandlePictureDecode(void *pUserData, CUVIDPICPARAMS *pPicParams)
{
    //bool bFrameAvailable = ((NvDecoder *)pUserData)->nvqueue->waitUntilFrameAvailable(pPicParams->CurrPicIdx);
    cuCtxPushCurrent(((NvDecoder *)pUserData)->nvcontext);
    timeval tv;
    gettimeofday(&tv, NULL);
    //fprintf(stderr, "Decode of index %d is called at: %ld,%ld\n",pPicParams->CurrPicIdx, tv.tv_sec, tv.tv_usec);
    CUresult oResult = cuvidDecodePicture(((NvDecoder *)pUserData)->nvdecoder, pPicParams);
    is_decoding[pPicParams->CurrPicIdx] = true;
    cuCtxPopCurrent(NULL);
    if (oResult != CUDA_SUCCESS) {
        printf("???????????????????????CUDA ERROR DECODING of Decoder.\n");
    }
    return 1;
}

int videoparser::HandlePictureDisplay(void *pUserData, CUVIDPARSERDISPINFO *pDispInfo)
{
    //((NvDecoder *)pUserData)->nvqueue->enqueue(pDispInfo);
    //printf("id is %d.\n",pDispInfo->picture_index);
    return 1;
}

void NvDecoder::init(FIFO ** input, FIFO ** output, int input_cnt, int output_cnt, int cuda_device, CUcontext * ctx)
{
    this->input_cnt = input_cnt;
    this->input = input;
    this->output_cnt = output_cnt;
    this->output = output;
    this->cnt_in = 0;
    cnt_out = 0;
    this->nvDev_id = cuda_device;
    this->stop = false;
    this->sender_stop = false;

    //init cuda ctx
    CUvideoctxlock g_CtxLock = NULL;
    void * hHandleDriver = 0;

    uint8_t *pHostPtr;
    //cuInit(0);
    //cuInit(0, __CUDA_API_VERSION, hHandleDriver);
    //cuvidInit(0);
    this->nvcontext = *ctx;
    //cuCtxCreate(&this->nvcontext, CU_CTX_SCHED_SPIN, nvdevice);
    cuvidCtxLockCreate(&g_CtxLock, this->nvcontext);

    //init decoder
    CUVIDDECODECREATEINFO stDecodeCreateInfo;
    memset(&stDecodeCreateInfo, 0, sizeof(CUVIDDECODECREATEINFO));
    stDecodeCreateInfo.ulWidth = 3840;
    stDecodeCreateInfo.ulHeight = 2160;
    stDecodeCreateInfo.ulNumDecodeSurfaces = MAX_DEC_SURF;
    stDecodeCreateInfo.CodecType  = cudaVideoCodec_HEVC;
    stDecodeCreateInfo.ChromaFormat = cudaVideoChromaFormat_420;
    stDecodeCreateInfo.ulCreationFlags = cudaVideoCreate_Default;
    stDecodeCreateInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    stDecodeCreateInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
    stDecodeCreateInfo.ulTargetWidth = 3840;
    stDecodeCreateInfo.ulTargetHeight = 2160;
    stDecodeCreateInfo.ulNumOutputSurfaces = 1;
    stDecodeCreateInfo.vidLock = g_CtxLock;

    cuCtxPushCurrent(this->nvcontext);
    cuvidCreateDecoder(&this->nvdecoder, &stDecodeCreateInfo);
    cuCtxPopCurrent(NULL);
    for(int i = 0 ; i < MAX_DEC_SURF ; i ++) {
        is_decoding[i] = false;
    }

    //init parser
    this->nvparser.init(this);

    //init frame queue
    //this->nvqueue = new FrameQueue();
}


void NvDecoder::loop_nvdecoder_send()
{
    long long enc_data_len;
    CUVIDSOURCEDATAPACKET packet;
    this->fcnt_in = 0;

    while (true) {
        if (!this->input[0]->get(this->data_buf, enc_data_len, this->cnt_in, false, 50)) {
            if (sys_f_cnt > 0 && this->fcnt_in == sys_f_cnt) {
                packet.flags = CUVID_PKT_ENDOFSTREAM;
                packet.payload = NULL;
                packet.payload_size = 0;
                cuCtxPushCurrent(this->nvcontext);
                cuvidParseVideoData(this->nvparser.nvparser_, &packet);
                cuCtxPopCurrent(NULL);
                break;
            } else {
                continue;
            }
        }
        packet.payload_size = enc_data_len;
        packet.payload = this->data_buf;
        packet.flags = CUVID_PKT_ENDOFPICTURE;
        cuCtxPushCurrent(this->nvcontext);
        cuvidParseVideoData(this->nvparser.nvparser_, &packet);
        cuCtxPopCurrent(NULL);
        timeval tv;
        gettimeofday(&tv, NULL);
        //fprintf(stderr, "Decode of frame %d is called at: %ld,%ld\n",this->fcnt_in, tv.tv_sec, tv.tv_usec);
        this->fcnt_in ++;
    }
    this->sender_stop = true;
    while(!this->stop) {
        usleep(10000);
    }
    //delete this->nvqueue;
    cuvidDestroyVideoParser(this->nvparser.nvparser_);
	fflush(stdout);
}

void NvDecoder::loop_nvdecoder_receive()
{
    this->fcnt_out = 0;
    CUdeviceptr cuDevPtr = 0;
    unsigned int nPitch = 0;
    CUdeviceptr pDevPtr;
    uint8_t *pHostPtr;
    CUresult rResult;
    CUVIDPARSERDISPINFO pDispInfo;
    CUVIDPROCPARAMS oVideoProcessingParameters;

    CUVIDGETDECODESTATUS status;

    cuCtxPushCurrent(this->nvcontext);
    cuMemAlloc(&pDevPtr, MAX_FIFO_SIZE);
    cuMemAllocHost((void** )(&pHostPtr), FRAME_SIZE);

    int wait_cnt = 0;

    int current_id;

    int nPPitch = 15360, iMatrix = 2;

    int shift;
    int scale;
    int viewIdx;
    CUdeviceptr dyvFrame;
    cuMemAlloc(&dyvFrame, FRAME_SIZE);
    CUdeviceptr dselectedFrame;
    cuMemAlloc(&dselectedFrame, FRAME_SIZE / 16);
    CUdeviceptr dupsampledFrame;
    cuMemAlloc(&dupsampledFrame, FRAME_SIZE / 4);


    FramePresenterGLX gInstance(1920, 1080);
    CUdeviceptr dpFrame;

    FILE* outfile = fopen("../outyuv_1920x1080.yuv", "wb");
    uint8_t* outyuv = (uint8_t*)malloc(FRAME_SIZE * 30 / 4);


    while (true) {
        current_id = cnt_out % MAX_DEC_SURF;
        if (this->sender_stop) {
            break;
        }
        if (!is_decoding[current_id]) {
            usleep(500);
            continue;
        }
        CUresult res = cuvidGetDecodeStatus(this->nvdecoder, current_id, &status);
        //printf("status_test, frame %d, %d.\n", (int) cnt_out + 1, status.decodeStatus);
        fflush(stdout);
        if (status.decodeStatus==cuvidDecodeStatus_Success){//(nvqueue->dequeue(&pDispInfo)) {//when there is frame decoded
            this->fcnt_out ++;
            //printf("Frame %d decoding.\n",this->fcnt_out);
            wait_cnt = 0;
            if (true) {//get output
                memset(&oVideoProcessingParameters, 0, sizeof(CUVIDPROCPARAMS));
                oVideoProcessingParameters.progressive_frame = pDispInfo.progressive_frame;
                oVideoProcessingParameters.second_field      = 0;
                oVideoProcessingParameters.top_field_first   = pDispInfo.top_field_first;
                oVideoProcessingParameters.unpaired_field    = 1;

                rResult = cuvidMapVideoFrame(this->nvdecoder, current_id, &cuDevPtr, &nPitch, &oVideoProcessingParameters);
                // use CUDA based Device to Host memcpy

                if (rResult == CUDA_SUCCESS){
                    fprintf(stderr,"test for frame %d.\n",this->fcnt_out);
                    cuCtxPushCurrent(this->nvcontext);
                    gInstance.GetDeviceFrameBuffer(&dpFrame, &nPPitch);
                    my_convert_nv12_to_yv12((uint8_t *)cuDevPtr, (uint8_t *)dyvFrame, this->nvDev_id);
                    //shift = fcnt_out % 23 + 1;
                    viewIdx = viewIdx_selector(0, 1);
                    scale = (viewIdx <= 8) ? 2 : 4;
                    view_selector_gpu((uint8_t *)dyvFrame, (uint8_t *)dselectedFrame, viewIdx, this->nvDev_id);
                    my_bicubic_yv12_upsample((uint8_t *)dselectedFrame, (uint8_t *)dupsampledFrame, scale, this->nvDev_id);
                    //cuMemcpyDtoH(outyuv, dupsampledFrame, FRAME_SIZE / 4);
                    //fwrite(outyuv, 1, FRAME_SIZE / 4, outfile);
                    my_convert_yv12_to_bgra_HD((uint8_t *)dupsampledFrame, (uint8_t *)dpFrame, this->nvDev_id);
                    cuCtxPopCurrent(NULL);
                    cuCtxPushCurrent(this->nvcontext);
                    gInstance.ReleaseDeviceFrameBuffer();
                    cuvidUnmapVideoFrame(this->nvdecoder, cuDevPtr);
                    cuCtxPopCurrent(NULL);
                    cnt_out ++;
                    int fifo_handler;
                    uint8_t * fifo_data_ptr;
                    int cnt_tmp;
                    long long len_tmp;
                    fifo_data_ptr = this->output[0]->fetch_put_pointer(FRAME_SIZE, cnt_out, fifo_handler);
                    //cuMemcpyDtoH(fifo_data_ptr, pDevPtr, FRAME_SIZE);
                    this->output[0]->release_put_pointer(fifo_handler);
                    fifo_data_ptr = this->output[0]->fetch_get_pointer(len_tmp, cnt_tmp, fifo_handler);
                    this->output[0]->release_get_pointer(fifo_handler);
                }
            }
            is_decoding[current_id] = false;
            //this->nvqueue->releaseFrame(&pDispInfo);
            if (sys_f_cnt > 0 && this->fcnt_out >= sys_f_cnt) break;
        }
        else {
            wait_cnt ++;
            usleep(500);
        }
    }
    cuMemFreeHost(pHostPtr);
    cuMemFree(pDevPtr);
    cuCtxPopCurrent(NULL);
    this->stop = true;
}

int NvDecoder::viewIdx_selector(int c, int shift)
{
    int leftIdx[TILE_LEFT], rightIdx[TILE_RIGHT];
    if(c < 4){
        for(int i = 0; i < c; i++){
            leftIdx[i] = c - (i+1);
        }
        for(int i = 0; i < 8 - c; i++){
            if(i < 4){
                rightIdx[i] = c + (i+1);
            }else{
                leftIdx[c+i-4] = c + (i+1);
            }
        }
        for(int i = 0; i < 8; i++){
            rightIdx[4+i] = 8 + (i+1);
            leftIdx[4+i] = 16 + (i+1);
        }
    }else if(c < 12){
        for(int i = 0; i < 4; i++){
            leftIdx[i] = c - (i+1);
            rightIdx[i] = c + (i+1);
        }
        for(int i = 0; i < c-4; i++){
            leftIdx[4+i] = c - 4 - (i+1);
        }
        for(int i = 0; i < 16 - (c-4); i++){
            if(i < 8){
                rightIdx[4+i] = c + 4 + (i+1);
            }else{
                leftIdx[c+i-8] = c + 4 + (i+1);
            }
        }
    }else if((c > VIEW_NUM - 13)&&(c <= VIEW_NUM - 5)){
        for(int i = 0; i < 4; i++){
            leftIdx[i] = c - (i+1);
            rightIdx[i] = c + (i+1);
        }
        for(int i = 0; i < VIEW_NUM - c - 5; i++){
            rightIdx[4+i] = c + 4 + (i+1);
        }
        for(int i = 0; i < 16 - (VIEW_NUM-c-5); i++){
            if(i < 8){
                leftIdx[4+i] = c - 4 - (i+1);
            }else{
                rightIdx[VIEW_NUM-9-c+i] = c - 4 - (i+1);
            }
        }
    }else if(c > VIEW_NUM - 5){
        for(int i = 0; i < (VIEW_NUM-1) - c; i++){
            rightIdx[i] = c + (i+1);
        }
        for(int i = 0; i < 8 - (VIEW_NUM-1-c); i++){
            if(i < 4){
                leftIdx[i] = c - (i+1);
            }else{
                rightIdx[(VIEW_NUM-1-c) + i - 4] = c - (i+1);
            }
        }
        for(int i = 0; i < 8; i++){
            leftIdx[4+i] = VIEW_NUM - 9 - (i+1);
            rightIdx[4+i] = VIEW_NUM - 17 - (i+1);
        }
    }else{
        for(int i = 0; i < 4; i++){
            leftIdx[i] = c - (i+1);
            rightIdx[i] = c + (i+1);
        }
        for(int i = 0; i < 8; i++){
            leftIdx[4+i] = c - 4 - (i+1);
            rightIdx[4+i] = c + 4 + (i+1);
        }
    }


    int view_now = (c + shift < 0) ? 0 : ((c + shift > VIEW_NUM - 1)  ? VIEW_NUM - 1 : c + shift);
    int viewIdx;
    if (shift == 0) {
        viewIdx = 0;
    } else {
        for (int i = 0; i < TILE_LEFT; i++) {
            if (leftIdx[i] == view_now) {
                viewIdx = 2 * i + 1;
                break;
            } else if (rightIdx[i] == view_now) {
                viewIdx = 2 * i + 2;
                break;
            }
        }
    }
    printf("\tviewIdx=%d\n", viewIdx);
    return viewIdx;
}