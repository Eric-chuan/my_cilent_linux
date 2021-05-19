#include "cvt.cuh"
#include <cstdio>
#include <stdio.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

__global__ void convert_nv12_to_bgra(uint8_t * nv12_input, uint8_t * rgba_output)
{
    int y = blockIdx.x * blockDim.x + threadIdx.x;
    int x = blockIdx.y * blockDim.y + threadIdx.y;
    int Y = nv12_input[y * 4096 + x];
    int U = nv12_input[2160 * 4096 + (y / 2) * 4096 + 2 * (x / 2)];
    int V = nv12_input[2160 * 4096 + (y / 2) * 4096 + 2 * (x / 2) + 1];
    int R = (298*Y + 411 * V - 57344)>>8;
    int G = (298*Y - 101* U - 211* V+ 34739)>>8;
    int B = (298*Y + 519* U- 71117)>>8;
    int A = 255;
    rgba_output[y * 3840 * 4 + 4 * x] = (uint8_t)((B < 0) ? 0 : ((B > 255) ? 255 : B));
    rgba_output[y * 3840 * 4 + 4 * x + 1] = (uint8_t)((G < 0) ? 0 : ((G > 255) ? 255 : G));
    rgba_output[y * 3840 * 4 + 4 * x + 2] = (uint8_t)((R < 0) ? 0 : ((R > 255) ? 255 : R));
    rgba_output[y * 3840 * 4 + 4 * x + 3] = (uint8_t)A;
}

void my_convert_nv12_to_bgra(uint8_t * input, uint8_t * output, int dev_id)
{
    cudaSetDevice(dev_id);
    dim3 Block(16, 64);
    dim3 Grid(135, 60);
    convert_nv12_to_bgra <<< Grid, Block >>> (input, output);
    //printf("CVT__called.\n");
    //cudaDeviceSynchronize();
}
