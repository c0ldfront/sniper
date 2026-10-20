#include "pickup_collect.h"
#include <string.h>
void sniper_pickup_collect(SniperPickupWorld *w,int32_t x,int32_t y,
    int32_t collector,int32_t code) {
    for(int32_t i=0;i<w->count;) {
        SniperPickupObject *p=w->items[i];
        if(p->x!=x || p->y!=y) { ++i; continue; }
        w->map[p->y*1024+p->x]=0;
        w->release(w->user,p);
        --w->count;
        memmove(w->items+i,w->items+i+1,(size_t)(w->count-i)*sizeof *w->items);
    }
    if(!code) return;
    SniperShipObject *local=w->local;
    if(local->word[0x44/4]!=collector && local->word[0x6c/4]!=collector) {
        if(!w->sharing_enabled || local->word[0xa0/4]>=w->sharing_limit) return;
        SniperShipObject *other=w->lookup(w->user,collector);
        if(!other || other->word[0x68/4]!=local->word[0x68/4]) return;
        uint32_t magnitude=code<0 ? 0u-(uint32_t)code : (uint32_t)code;
        if(magnitude==7 || magnitude==17 || magnitude==18 || magnitude==14) return;
        int32_t tx=local->word[0x48/4]/16000,ty=local->word[0x4c/4]/16000;
        if(tx>=0 && ty>=0 && tx<1024 && ty<1024 && w->map[ty*1024+tx]==152) return;
    }
    w->award(w->user,local,code);
}
