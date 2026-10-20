#include "list_navigation.h"
#include <string.h>
static int32_t add(int32_t value,uint32_t delta) {
    uint32_t bits=(uint32_t)value+delta;int32_t result;memcpy(&result,&bits,4);return result;
}
int sniper_list_navigation(int32_t key,int32_t shift,int32_t count,int32_t height,
    int32_t *selected,int32_t *history,int32_t *dirty) {
    if(key!=0x21 && key!=0x22)return 0;
    if(key==0x21) {
        if(shift&0x8000){int32_t n=add(*history,UINT32_MAX);*history=n>1?n:1;}
        else {int32_t n=add(*selected,UINT32_MAX);*selected=n<0?0:n;}
    } else {
        if(shift&0x8000){int32_t n=add(*history,1),limit=height/24;*history=limit<n?limit:n;}
        else {int32_t n=add(*selected,1);*selected=count>n?n:add(count,UINT32_MAX);}
    }
    *dirty=1;return 1;
}
