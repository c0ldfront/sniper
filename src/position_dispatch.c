#include "position_dispatch.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
#define S(o) w->local->word[(o)/4]
int sniper_position_dispatch(SniperPositionDispatch *w,const int32_t weapon[6]){
    int32_t interval=w->interval;
    if(S(0x6c)!=-1)interval=bits((uint32_t)interval*4u);
    else if(S(0x50)==w->last_vx && S(0x54)==w->last_vy && S(0x58)==w->last_heading){
        interval=bits((uint32_t)interval*2u);
        /* Original signed comparisons do not take absolute velocity. */
        if(S(0x50)<500 && S(0x54)<500)interval=bits((uint32_t)interval*2u);
    }
    if(!weapon[0] && bits((uint32_t)w->ticks-(uint32_t)w->last_ticks)<interval)return 0;
    int32_t flags=(S(0x88)?1:0)|(S(0x8c)?2:0)|(S(0x94)?4:0)|(S(0x90)?8:0);
    if(w->position_dirty){w->position_dirty=0;flags|=16;}
    int32_t args[13]={S(0x48)/1000,S(0x4c)/1000,S(0x50),S(0x54),S(0x58)/1000,
        S(0x48)==-1?0:weapon[0],weapon[1],weapon[2],weapon[3],weapon[4],weapon[5],flags,S(0xa0)};
    w->send(w->user,args);
    w->last_ticks=w->ticks;w->last_vx=S(0x50);w->last_vy=S(0x54);w->last_heading=S(0x58);
    return 1;
}
