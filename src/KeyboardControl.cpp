//
// Created by hjc on 5/25/21.
//
#include "KeyboardControl.h"

void KeyboardControl::init(Context *ctx)
{
    this->ctx = ctx;
    fflush(stdout);
    struct termios orig_termios;
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    new_termios.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &new_termios);
}
void KeyboardControl::loop()
{
    while(true){
		struct timeval tv = {0L, 0L};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        if(select(1, &fds, NULL, NULL, &tv)){
			keyboard_in = getchar();
            switch(keyboard_in){
                case 118:
                    this->ctx->centerIdx = (this->ctx->centerIdx - 1 < 0) ? 0 : this->ctx->centerIdx - 1;
                    break;
                case 98:
                    this->ctx->centerIdx = (this->ctx->centerIdx + 1 > 24) ? 24 : this->ctx->centerIdx + 1;
                    break;
                default:
                    break;
            }
        }
        fflush(stdout);
    }
}