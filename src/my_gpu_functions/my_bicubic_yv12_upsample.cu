#include "cvt.cuh"
#include <cstdio>
#include <stdio.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

inline __device__ float fastMax(const float a, const float b)
{
    return (a > b ? a : b);
}

inline __device__ float fastMin(const float a, const float b)
{
    return (a < b ? a : b);
}

inline __device__ float fastTruncate(float value, float min = 0.0f, float max = 1.0f)
{
    return fastMin(max, fastMax(min, value));
}

inline __device__ void cubicSequentialData(int* xIntArray, int* yIntArray, float& dx, float& dy, const float xSource, const float ySource, int scale)
{
    int width = (scale == 2) ? 960 : 480;
    int height = (scale == 2) ? 540 : 270;
    xIntArray[1] = fastTruncate(int(xSource + 1e-5), 0, width - 1);
    xIntArray[0] = fastMax(0, xIntArray[1] - 1);
    xIntArray[2] = fastMin(width - 1, xIntArray[1] + 1);
    xIntArray[3] = fastMin(width - 1, xIntArray[2] + 1);
    dx = xSource - xIntArray[1];

    yIntArray[1] = fastTruncate(int(ySource + 1e-5), 0, height - 1);
    yIntArray[0] = fastMax(0, yIntArray[1] - 1);
    yIntArray[2] = fastMin(height - 1, yIntArray[1] + 1);
    yIntArray[3] = fastMin(height - 1, yIntArray[2] + 1);
    dy = ySource - yIntArray[1];
}

inline __device__ void cubicSequentialDataUV(int* xIntArray, int* yIntArray, float& dx, float& dy, const float xSource, const float ySource, int scale)
{
    int width = (scale == 2) ? 480 : 240;
    int height = (scale == 2) ? 270 : 135;
    xIntArray[1] = fastTruncate(int(xSource + 1e-5), 0, width - 1);
    xIntArray[0] = fastMax(0, xIntArray[1] - 1);
    xIntArray[2] = fastMin(width - 1, xIntArray[1] + 1);
    xIntArray[3] = fastMin(width - 1, xIntArray[2] + 1);
    dx = xSource - xIntArray[1];

    yIntArray[1] = fastTruncate(int(ySource + 1e-5), 0, height - 1);
    yIntArray[0] = fastMax(0, yIntArray[1] - 1);
    yIntArray[2] = fastMin(height - 1, yIntArray[1] + 1);
    yIntArray[3] = fastMin(height - 1, yIntArray[2] + 1);
    dy = ySource - yIntArray[1];
}

inline __device__ float cubicInterpolation(const float v0, const float v1, const float v2, const float v3, const float dx)
{
    return (-0.5f * v0 + 1.5f * v1 - 1.5f * v2 + 0.5f * v3) * dx * dx * dx
            + (v0 - 2.5f * v1 + 2.f * v2 - 0.5f * v3) * dx * dx
            - 0.5f * (v0 - v2) * dx // + (-0.5f * v0 + 0.5f * v2) * dx
            + v1;
}
inline __device__ float bicubicInterpolate(uint8_t* src, float xSource, float ySource, int scale)
{
    int width = (scale == 2) ? 960 : 480;
    int xIntArray[4];
    int yIntArray[4];
    float dx;
    float dy;
    cubicSequentialData(xIntArray, yIntArray, dx, dy, xSource, ySource, scale);

    float temp[4];
    for (int i = 0; i < 4; i++)
    {
        const int offset = yIntArray[i] * width;
        temp[i] = cubicInterpolation((float)(src[offset + xIntArray[0]]), (float)(src[offset + xIntArray[1]]),
                                    (float)(src[offset + xIntArray[2]]), (float)(src[offset + xIntArray[3]]), dx);
    }
    return cubicInterpolation(temp[0], temp[1], temp[2], temp[3], dy);
}

inline __device__ float bicubicInterpolateUV(uint8_t* src, float xSource, float ySource, int scale)
{
    int width = (scale == 2) ? 480 : 240;
    int xIntArray[4];
    int yIntArray[4];
    float dx;
    float dy;
    cubicSequentialDataUV(xIntArray, yIntArray, dx, dy, xSource, ySource, scale);

    float temp[4];
    for (int i = 0; i < 4; i++)
    {
        const int offset = yIntArray[i] * width;
        temp[i] = cubicInterpolation((float)(src[offset + xIntArray[0]]), (float)(src[offset + xIntArray[1]]),
                                    (float)(src[offset + xIntArray[2]]), (float)(src[offset + xIntArray[3]]), dx);
    }
    return cubicInterpolation(temp[0], temp[1], temp[2], temp[3], dy);
}


__global__ void bicubic_upsample_kernel(uint8_t* src,  uint8_t* dst, int scale)
{
    int x = threadIdx.x + blockIdx.x * blockDim.x;  //0~1920
    int y = threadIdx.y + blockIdx.y * blockDim.y;  //0~1080
    int xDim = blockDim.x * gridDim.x;  //1920
    int yDim = blockDim.y * gridDim.y;  //1080
    int xDim1 = xDim / 2;
    int yDIm1 = yDim / 2;
    int start_u = xDim * yDim;
    int start_v = start_u * 5 / 4;

    float xSource = (x + 0.5f) / scale - 0.5f;
    float ySource = (y + 0.5f) / scale - 0.5f;
    dst[x + y * xDim] = (uint8_t)(bicubicInterpolate(src, xSource, ySource, scale));
    if ((x % 2 == 0) && (y % 2 == 0)) {
        int x1 = x / 2;
        int y1 = y / 2;
        float xSource1 = (x1 + 0.5f) / scale - 0.5f;
        float ySource1 = (y1 + 0.5f) / scale - 0.5f;
        dst[start_u + x1 + y1 * xDim1] = (uint8_t)(bicubicInterpolateUV(&src[start_u / scale / scale], xSource1, ySource1, scale));
        dst[start_v + x1 + y1 * xDim1] = (uint8_t)(bicubicInterpolateUV(&src[start_v / scale / scale], xSource1, ySource1, scale));
    }
}

void my_bicubic_yv12_upsample(uint8_t* src,  uint8_t* dst, int scale, int dev_id)
{
    cudaSetDevice(dev_id);
    dim3 blocks(1920 / 32, 1080 / 20);
    dim3 threads(32, 20);
    bicubic_upsample_kernel <<< blocks, threads >>> (src, dst, scale);
}
