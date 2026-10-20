#include "death_event.h"
#include <stdio.h>
#include <string.h>
static int32_t bits(uint32_t x){int32_t r;memcpy(&r,&x,4);return r;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
void sniper_death_event(SniperDeathWorld *w,int32_t killer,int32_t victim,int32_t points,int32_t prize){
    SniperShipObject *k=w->players[killer],*v=w->players[victim];
    SniperPickupWorld *pickups=w->pickups;
    if(prize>=0 && pickups->count<16 && v && (v->word[0x50/4] || v->word[0x54/4])){
        int32_t x=v->word[0x48/4]/16000,y=v->word[0x4c/4]/16000;
        if(x>=0 && y>=0 && x<1024 && y<1024 && !pickups->map[y*1024+x]){
            SniperPickupObject *p=w->allocate(w->user);
            if(p){*p=(SniperPickupObject){0,x,y,w->pickup_lifetime,prize};pickups->map[y*1024+x]=255;}
            /* Original appends NULL on allocation failure, retained here. */
            pickups->items[pickups->count++]=p;
        }
    }
    if(k){int32_t score=add(k->word[0x80/4],points);k->word[0x80/4]=score<0?0:score;}
    if(killer!=victim && k){
        k->word[0x78/4]=add(k->word[0x78/4],1);
        if(k==w->local)w->local->word[0xa0/4]=add(w->local->word[0xa0/4],w->kill_bounty);
    }
    if(v){
        v->word[0x7c/4]=add(v->word[0x7c/4],1);
        if(victim!=w->local_id)w->damage(w->user,v,-1,killer);
    }
    if(w->local->word[0x6c/4]==victim)w->detach(w->user,-1);
    if(k && v){
        char text[128];
        if(killer==victim)snprintf(text,sizeof text,"%s(%d) killed by self",(char *)v,points);
        else snprintf(text,sizeof text,"%s(%d) killed by: %s",(char *)v,points,(char *)k);
        sniper_notice_push(w->notices,text,400,k==w->local?2:3);
    }
}
void sniper_offline_death(SniperDeathWorld *w,int32_t killer,int32_t bounty){
    sniper_death_event(w,killer,0,bits(0u-(uint32_t)bounty),-1);
}
