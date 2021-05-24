cmake_minimum_required(VERSION 3.5)
project(my_client_linux)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I. -Wall -Werror -pedantic -fsanitize=address -g")
#SET(QUIC_LIB_DIR ${PROJECT_SOURCE_DIR}/deps/quic-hevc/examples/build/debug)

# set(CMAKE_BUILD_TYPE Release)
#set(THREADS_PREFER_PTHREAD_FLAG ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Os -Wall -pedantic")

#find_package(SDL2 REQUIRED)

#find_package(glfw3 REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#dy add for cuda
FIND_PACKAGE(CUDA)
set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
#SET(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_61,code=sm_61;-std=c++11;)

add_definitions(-DCMAKE)
#dy modified for cuda
#file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/include/opengl/glad/src/glad.cpp" "${PROJECT_SOURCE_DIR}/src/*.cu")
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# inlcude path
include_directories(include)
#include_directories("include/opengl")
#include_directories("include/opengl/glad/include")
#include_directories(${PROJECT_SOURCE_DIR}/deps/quic-hevc/include)
#include_directories(${PROJECT_SOURCE_DIR}/deps/SVT-HEVC/Source/API)
#include_directories(include/nvdecoder)
include_directories(include/nvdecoder)
include_directories(include/my_gpu_functions)
include_directories(include/FramePresenterGLX)
#dy add for cuda
include_directories("/usr/local/cuda/include")
link_directories("/usr/local/cuda/lib64")
#

# # include path pullstream
include_directories(/usr/include)

# include SVT-HEVC library
#SET(SVT-HEVC_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/deps/SVT-HEVC/Bin/Release)
#LINK_LIBRARIES(${SVT-HEVC_LIBRARY_PATH}/libSvtHevcEnc.so)
#LINK_LIBRARIES("/home/enc/low_delay_sys/git/my_host_linux/deps/SVT-HEVC/Bin/Release/libSvtHevcEnc.so")
#SET(SVT-HEVC_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/deps/SVT-HEVC)
#include_directories(${SVT-HEVC_LIBRARY_PATH})
#include_directories(${SVT-HEVC_LIBRARY_PATH}/Bin/Release)
#include_directories(${SVT-HEVC_LIBRARY_PATH}/Source/API)

#include my_gpu_functions library
SET(MY_GPU_FUNCTION_PATH ${PROJECT_SOURCE_DIR}/src/my_gpu_functions/build)
LINK_LIBRARIES(${MY_GPU_FUNCTION_PATH}/libmy_gpu_functions.so)
LINK_LIBRARIES(${PROJECT_SOURCE_DIR}/lib/libnvcuvid.so)


#link quiche directories
#link_directories(${QUIC_LIB_DIR})

# subdirectory
#add_subdirectory(${PROJECT_SOURCE_DIR}/src/nvdecoder)
#add_subdirectory(${PROJECT_SOURCE_DIR}/src/my_gpu_functions)
find_library(FREEGLUT_LIB glut)
find_library(GLEW32_LIB GLEW)
find_library(X11_LIB X11)
find_library(GL_LIB GL)
find_library(CUDART_LIB cudart HINTS ${CUDA_TOOLKIT_ROOT_DIR}/lib64)

add_executable(my_client_linux ${SOURCES})

target_link_libraries(my_client_linux
        Threads::Threads
        cuda
        -lcurl
        ${CUDART_LIB} ${FREEGLUT_LIB} ${GLEW32_LIB} ${X11_LIB} ${GL_LIB})


