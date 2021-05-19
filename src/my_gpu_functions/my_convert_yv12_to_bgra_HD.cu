#include "cvt.cuh"
#include <cstdio>
#include <stdio.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

__global__ void convert_yv12_to_bgra_HD_kernel(uint8_t * yv12_input, uint8_t * rgba_output)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int Y = yv12_input[y * 1920 + x];
    int V = yv12_input[1920 * 1080 + (x / 2) + 960 * (y / 2)];
    int U = yv12_input[1920 * 1080 * 5 / 4 + (x / 2) + 960 * (y / 2)];
    int R = (298*Y + 411 * V - 57344)>>8;
    int G = (298*Y - 101* U - 211* V+ 34739)>>8;
    int B = (298*Y + 519* U- 71117)>>8;
    int A = 255;
    rgba_output[y * 1920 * 4 + 4 * x] = (uint8_t)((B < 0) ? 0 : ((B > 255) ? 255 : B));
    rgba_output[y * 1920 * 4 + 4 * x + 1] = (uint8_t)((G < 0) ? 0 : ((G > 255) ? 255 : G));
    rgba_output[y * 1920 * 4 + 4 * x + 2] = (uint8_t)((R < 0) ? 0 : ((R > 255) ? 255 : R));
    rgba_output[y * 1920 * 4 + 4 * x + 3] = (uint8_t)A;
}

void my_convert_yv12_to_bgra_HD(uint8_t * input, uint8_t * output, int dev_id)
{
    cudaSetDevice(dev_id);
    dim3 Block(32, 20);
    dim3 Grid(1920 / 32, 1080 / 20);
    convert_yv12_to_bgra_HD_kernel <<< Grid, Block >>> (input, output);
    //printf("CVT__called.\n");
    //cudaDeviceSynchronize();
}
