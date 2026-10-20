#include "ship_visual.h"
int sniper_ship_visual(SniperShipObject *o,uint32_t *blink,int low_detail,
    int32_t local,int field94,int32_t cx,int32_t cy,
    SniperEffectDraw effects,SniperBodyDraw body,void *ctx) {
    if(o->word[0x48/4]==-1 || o->word[0xa4/4]==2) return 0;
    if(o->word[0xa4/4]==1) {
        uint32_t previous=(*blink)++;
        if(previous&3u) return 0;
    }
    if(!low_detail) sniper_ship_effect_draw(o,local,cx,cy,effects,ctx);
    sniper_ship_body_draw(o,local,field94,cx,cy,body,ctx);
    return 1;
}
