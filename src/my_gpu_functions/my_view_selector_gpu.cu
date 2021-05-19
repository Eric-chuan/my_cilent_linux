#include "cvt.cuh"
#include <cstdio>
#include <stdio.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

__global__ void view_selector_kernel(uint8_t* mvstream_data,  uint8_t* selected_view_data, int viewIdx)
{
    int x = threadIdx.x + blockIdx.x * blockDim.x;  //0~960
    int y = threadIdx.y + blockIdx.y * blockDim.y;  //0~540
    int xDim = blockDim.x * gridDim.x;  //960
    int yDim = blockDim.y * gridDim.y;  //540
    int xDim1 = xDim / 2;
    int yDim1 = yDim / 2;
    int xDim2 = xDim / 4;
    int yDim2 = yDim / 4;

    int start_u = xDim * yDim;
    int start_v = xDim * yDim * 5 / 4;
    //center
    if (viewIdx == 0) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                selected_view_data[x + i * xDim + (y + j * yDim) * 2 * xDim] = mvstream_data[x + i * xDim + (y + j * yDim) * 4 * xDim];
            }
        selected_view_data[start_u * 4 + x + y * xDim] = mvstream_data[start_u * 16 + x + y * 2 * xDim];
        selected_view_data[start_v * 4 + x + y * xDim] = mvstream_data[start_v * 16 + x + y * 2 * xDim];
        }
    } else if (viewIdx <= 8) {
        if (viewIdx % 2 == 1) {
            selected_view_data[x + y * xDim] = mvstream_data[x + (2 + (viewIdx % 4) / 2) * xDim + (y + viewIdx / 4 * yDim) * 4 * xDim];
            if ((x % 2 == 0)&&(y % 2 == 0)) {
                int x1 = x / 2;  //0~480
                int y1 = y / 2;  //0~270
                selected_view_data[start_u + x1 + y1 * xDim1] = mvstream_data[start_u * 16 + x1 + (2 + (viewIdx % 4) / 2) * xDim1 + (y1 + viewIdx / 4 * yDim1) * xDim1 * 4];
                selected_view_data[start_v + x1 + y1 * xDim1] = mvstream_data[start_v * 16 + x1 + (2 + (viewIdx % 4) / 2) * xDim1 + (y1 + viewIdx / 4 * yDim1) * xDim1 * 4];
            }
        } else {
            selected_view_data[x + y * xDim] = mvstream_data[start_u * 8 + x + (((viewIdx - 1) % 4) / 2) * xDim + (y + (viewIdx - 1) / 4 * yDim) * 4 * xDim];
            if ((x % 2 == 0)&&(y % 2 == 0)) {
                int x1 = x / 2;  //0~480
                int y1 = y / 2;  //0~270
                selected_view_data[start_u + x1 + y1 * xDim1] = mvstream_data[start_u * 18 + x1 + (((viewIdx - 1) % 4) / 2) * xDim1 + (y1 + (viewIdx - 1) / 4 * yDim1) * xDim1 * 4];
                selected_view_data[start_v + x1 + y1 * xDim1] = mvstream_data[start_u * 22 + x1 + (((viewIdx - 1) % 4) / 2) * xDim1 + (y1 + (viewIdx - 1) / 4 * yDim1) * xDim1 * 4];
            }
        }
    } else {
        if (viewIdx % 2 == 1) {
            if ((x % 2 == 0)&&(y % 2 == 0)) {
                int x1 = x / 2;  //0~480
                int y1 = y / 2;  //0~270
                selected_view_data[x1 + y1 * xDim1] = mvstream_data[start_u * 8 + x1 + ((viewIdx / 2) % 4 + 4) * xDim1 + (y1 + (viewIdx / 8 - 1) * yDim1) * 4 * xDim];
            }
            if ((x % 4 == 0)&&(y % 4 == 0)) {
                int x2 = x / 4;
                int y2 = y / 4;
                selected_view_data[start_u / 4 + x2 + y2 * xDim2] = mvstream_data[start_u * 18 + x2 + ((viewIdx / 2) % 4 + 4) * xDim2 + (y2 + (viewIdx / 8 - 1) * yDim2) * xDim1 * 4];
                selected_view_data[start_v / 4 + x2 + y2 * xDim2] = mvstream_data[start_u * 22 + x2 + ((viewIdx / 2) % 4 + 4) * xDim2 + (y2 + (viewIdx / 8 - 1) * yDim2) * xDim1 * 4];
            }
        } else {
            if ((x % 2 == 0)&&(y % 2 == 0)) {
                int x1 = x / 2;  //0~480
                int y1 = y / 2;  //0~270
                selected_view_data[x1 + y1 * xDim1] = mvstream_data[start_u * 8 + x1 + (((viewIdx - 1) / 2) % 4 + 4) * xDim1 + (y1 + ((viewIdx - 1) / 8 + 1) * yDim1) * 4 * xDim];
            }
            if ((x % 4 == 0)&&(y % 4 == 0)) {
                int x2 = x / 4;
                int y2 = y / 4;
                selected_view_data[start_u / 4 + x2 + y2 * xDim2] = mvstream_data[start_u * 18 + x2 + (((viewIdx - 1) / 2) % 4 + 4) * xDim2 + (y2 + ((viewIdx - 1) / 8 + 1) * yDim2) * xDim1 * 4];
                selected_view_data[start_v / 4 + x2 + y2 * xDim2] = mvstream_data[start_u * 22 + x2 + (((viewIdx - 1) / 2) % 4 + 4) * xDim2 + (y2 + ((viewIdx - 1) / 8 + 1) * yDim2) * xDim1 * 4];
            }
        }
    }
}

void view_selector_gpu(uint8_t * input, uint8_t * output, int viewIdx, int dev_id)
{
    cudaSetDevice(dev_id);
    dim3 blocks(960 / 32, 540 / 20);
    dim3 threads(32, 20);
    view_selector_kernel <<< blocks, threads >>> (input, output, viewIdx);
}