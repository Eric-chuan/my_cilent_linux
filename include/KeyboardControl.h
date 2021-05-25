//
// Created by hjc on 5/25/21.
//

#ifndef MY_CLIENT_LINUX_KEYBOARD_CONTROL_H
#define MY_CLIENT_LINUX_KEYBOARD_CONTROL_H
#include "common.h"
#include "Module.h"
#include "Context.h"
#include <atomic>
#include <termios.h>

class KeyboardControl
{
private:
    Context *ctx;
    int keyboard_in;
    //timeval * tout;
public:
    void init(Context *ctx);
    void loop();
    int get_mem_cnt(){
        return 1;
    };
};

#endif //MY_CLIENT_LINUX_KEYBOARD_CONTROL_H