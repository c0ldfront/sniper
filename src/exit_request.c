#include "exit_request.h"
void sniper_exit_request(int32_t *mode,int32_t *pending,SniperNotices *notices) {
    if(*mode){*mode=0;return;}
    *pending=1;
    sniper_notice_push(notices,"Exiting game...",1500,2);
}
