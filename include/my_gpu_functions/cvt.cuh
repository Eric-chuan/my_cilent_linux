#ifndef MY_CLIENT_LINUX_CVT_CUH
#define MY_CLIENT_LINUX_CVT_CUH

#include <cstdint>

extern "C" void my_convert_nv12_to_yv12(uint8_t * input, uint8_t * output, int dev_id=0);
extern "C" void my_convert_nv12_to_bgra(uint8_t * input, uint8_t * output, int dev_id=0);
extern "C" void my_convert_yv12_to_bgra_HD(uint8_t * input, uint8_t * output, int dev_id=0);
extern "C" void view_selector_gpu(uint8_t * input, uint8_t * output, int viewIdx, int dev_id=0);
extern "C" void my_bicubic_yv12_upsample(uint8_t* src,  uint8_t* dst, int scale, int dev_id=0);
#endif