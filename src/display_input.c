#include "display_input.h"
#include <string.h>
void sniper_display_input(int32_t key,int32_t *label_mode,int32_t *stars,int32_t *ticker) {
    if(key==0x72) {
        uint32_t u=(uint32_t)*label_mode+1u;int32_t n;memcpy(&n,&u,4);*label_mode=n%3;
    } else if(key==0x75)*stars=!*stars;
    else if(key==0x7a)*ticker=!*ticker;
}
