#include "pickup_lifecycle.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
int sniper_pickup_create(SniperPickupWorld *w,int32_t x,int32_t y,int32_t code,
    int32_t lifetime,SniperPickupObject *(*allocate)(void *),void *owner) {
    if(w->count>=16 || x<0 || y<0 || x>=1024 || y>=1024 || w->map[y*1024+x])return 0;
    SniperPickupObject *p=allocate(owner);
    if(!p)return -1;
    *p=(SniperPickupObject){0,x,y,lifetime,code};
    w->map[y*1024+x]=255;w->items[w->count++]=p;
    return 1;
}
void sniper_pickup_expire(SniperPickupWorld *w,int32_t ticks) {
    for(int32_t i=0;i<w->count;) {
        SniperPickupObject *p=w->items[i];
        p->age=bits((uint32_t)p->age+(uint32_t)ticks);
        p->lifetime=bits((uint32_t)p->lifetime-(uint32_t)ticks);
        if(p->lifetime>0){++i;continue;}
        w->map[p->y*1024+p->x]=0;w->release(w->user,p);
        --w->count;
        memmove(w->items+i,w->items+i+1,(size_t)(w->count-i)*sizeof *w->items);
    }
}
