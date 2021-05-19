cmake_minimum_required(VERSION 3.5)


project(my_gpu_functions)
find_package(CUDA REQUIRED)

#$SET(CMAKE_CUDA_NVCC_COMPILER nvcc)
#SET(CUDA_NVCC_FLAGS ${CUDA_NVCC} -std=c++11;)
SET(CUDA_NVCC_FLAGS ${CUDA_NVCC};-gencode arch=compute_75,code=sm_75;-std=c++11;)

FIND_PACKAGE(CUDA REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


file(GLOB SOURCES "*.cu")
#file(GLOB HEADERS "../../include/my_gpu_functions/*.cuh")

include_directories("../../include/my_gpu_functions")
include_directories("/usr/local/cuda/include")

link_directories("/usr/local/cuda/lib64")

cuda_add_library(my_gpu_functions SHARED ${SOURCES})
