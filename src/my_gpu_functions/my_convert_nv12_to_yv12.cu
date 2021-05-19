#include "cvt.cuh"
#include <cstdio>
#include <stdio.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

__global__ void convert_nv12_to_yv12(uint8_t * nv12_input, uint8_t * yv12_output)
{
    int ty = threadIdx.x;
    int tx = threadIdx.y;
    int starty = blockIdx.x * blockDim.x;
    int startx = blockIdx.y * blockDim.y;
    //input
    yv12_output[3840 * (2 * (starty + ty)) + (2 * (startx + tx))] = nv12_input[4096 * (2 * (starty + ty)) + (2 * (startx + tx))];
    yv12_output[3840 * (2 * (starty + ty) + 1) + (2 * (startx + tx))] = nv12_input[4096 * (2 * (starty + ty) + 1) + (2 * (startx + tx))];
    yv12_output[3840 * (2 * (starty + ty)) + (2 * (startx + tx) + 1)] = nv12_input[4096 * (2 * (starty + ty)) + (2 * (startx + tx) + 1)];
    yv12_output[3840 * (2 * (starty + ty) + 1) + (2 * (startx + tx) + 1)] = nv12_input[4096 * (2 * (starty + ty) + 1) + (2 * (startx + tx) + 1)];
    int start_uv_input = 4096 * 2160;
    int start_u_output = 3840 * 2160;
    int start_v_output = 3840 * 2160 * 5 / 4;
    yv12_output[start_u_output + 1920 * (starty + ty) + (startx + tx)] = nv12_input[start_uv_input + 4096 * (starty + ty) + (2 * (startx + tx))];
    yv12_output[start_v_output + 1920 * (starty + ty) + (startx + tx)] = nv12_input[start_uv_input + 4096 * (starty + ty) + (2 * (startx + tx) + 1)];
}

void my_convert_nv12_to_yv12(uint8_t * input, uint8_t * output, int dev_id)
{
    cudaSetDevice(dev_id);
    dim3 Block(8, 32);
    dim3 Grid(135, 60);
    convert_nv12_to_yv12 <<< Grid, Block >>> (input, output);
    //printf("CVT__called.\n");
    //cudaDeviceSynchronize();
}
