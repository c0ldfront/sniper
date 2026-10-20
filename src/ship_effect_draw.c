#include "ship_effect_draw.h"
#include <string.h>
static int32_t sub(int32_t a,int32_t b) {
    uint32_t u=(uint32_t)a-(uint32_t)b;int32_t s;memcpy(&s,&u,4);return s;
}
void sniper_ship_effect_draw(const SniperShipObject *o,int32_t local,
    int32_t cx,int32_t cy,SniperEffectDraw draw,void *ctx) {
    for(unsigned i=0;i<12;++i) {
        const int32_t *p=o->word+0x154/4+i*4;
        if(p[2]<=0) continue;
        int resource;int32_t frame;
        if(p[3]) {resource=173;frame=sub(52,p[2])/2;}
        else {
            if(o->word[0x44/4]!=local) continue;
            resource=147;frame=sub(57,p[2])/3;
        }
        draw(ctx,resource,sub(p[0],cx),sub(p[1],cy),frame);
    }
}
