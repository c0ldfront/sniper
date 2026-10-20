#include "inactive_position.h"
#include <string.h>
static int32_t bits(uint32_t x){int32_t s;memcpy(&s,&x,4);return s;}
int sniper_inactive_position(SniperPositionDispatch *w) {
    if(bits((uint32_t)w->ticks-(uint32_t)w->last_ticks)<bits((uint32_t)w->interval*4u))return 0;
    int32_t *s=w->local->word;
    int32_t flags=(s[0x88/4]?1:0)|(s[0x8c/4]?2:0)|(s[0x94/4]?4:0)|(s[0x90/4]?8:0);
    if(w->position_dirty){w->position_dirty=0;flags|=16;}
    int32_t args[13]={s[0x48/4]/1000,s[0x4c/4]/1000,s[0x50/4],s[0x54/4],s[0x58/4]/1000,
        0,0,0,0,0,0,flags,s[0xa0/4]};
    w->send(w->user,args);
    w->last_ticks=w->ticks;w->last_vx=s[0x50/4];w->last_vy=s[0x54/4];w->last_heading=s[0x58/4];
    return 1;
}
