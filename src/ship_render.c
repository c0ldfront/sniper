#include "ship_render.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
void sniper_ship_render(SniperShipObject *o,const SniperShipObject *local,
    SniperShipObject *parent,uint32_t *blink,int detail,int mode,
    int32_t upper,int32_t lower,int32_t bonus,int32_t cx,int32_t cy,
    SniperEffectDraw effects,SniperBodyDraw body,SniperLabelDraw labels,void *ctx){
    if(!sniper_ship_visual(o,blink,detail,local->word[0x44/4],local->word[0x94/4],cx,cy,effects,body,ctx))return;
    int32_t x=bits((uint32_t)(o->word[0x48/4]/1000)-(uint32_t)cx);
    int32_t y=bits((uint32_t)(o->word[0x4c/4]/1000)-(uint32_t)cy);
    if(o->word[0x60/4]<=0 && !local->word[0x94/4]){
        x=bits((uint32_t)x+(uint32_t)o->word[0xb4/4]);
        y=bits((uint32_t)y+(uint32_t)o->word[0xb8/4]);
    }
    sniper_ship_labels(o,local,parent,mode,upper,lower,bonus,x,y,labels,ctx);
}
