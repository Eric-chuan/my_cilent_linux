//
// Created by dy on 6/29/20.
//

#ifndef MY_CLIENT_LINUX_COMMON_H
#define MY_CLIENT_LINUX_COMMON_H

#include <string.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <stdio.h>
#include <sys/time.h>
#include <semaphore.h>
#define MAX_LOG_CNT 10000
#define MAX_FIFO_SIZE (4096 * 2160 * 3 / 2)
#define FIFO_LEN 5
#define FRAME_SIZE (3840 * 2160 * 3 / 2)
#endif //MY_CLIENT_LINUX_COMMON_H
