#include "notices.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
void sniper_notice_push(SniperNotices *n,const char *text,int32_t ticks,int32_t color) {
    unsigned slot=0;while(slot<4 && n->ticks[slot]>0)++slot;
    if(slot==4)slot=0;
    memcpy(n->text[slot],text,strlen(text)+1);n->ticks[slot]=ticks;n->color[slot]=color;
}
void sniper_notices_step(SniperNotices *n,int32_t ticks) {
    for(unsigned i=0;i<4;++i)if(n->ticks[i]>0)n->ticks[i]=bits((uint32_t)n->ticks[i]-(uint32_t)ticks);
}
void sniper_notices_draw(const SniperNotices *n,int32_t width,int32_t height,SniperNoticeDraw draw,void *user) {
    int32_t y=bits((uint32_t)height*3u)/5;
    for(unsigned i=0;i<4;++i)if(n->ticks[i]>0)
        draw(user,n->text[i],width/6,bits((uint32_t)y+i*8u),n->color[i]);
}
