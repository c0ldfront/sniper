#include "player_registry.h"
#include <stdio.h>
#include <string.h>
SniperShipObject *sniper_player_add(SniperPlayerRegistry *w,const SniperShipIdentity *id) {
    SniperShipObject *s=w->allocate(w->user);
    if(!s)return NULL;
    sniper_ship_create_clocked(s,id,w->defaults,w->milliseconds,w->clock_ms,w->clock_user);
    w->players[id->player_id]=s;w->ordered[w->count++]=s;
    if(w->local) {
        char message[128];snprintf(message,sizeof message,"%s entering game",id->name);
        sniper_notice_push(w->notices,message,300,0);
        w->dirty=1;
        int32_t selected_id=-1;
        if(w->selected<w->count)selected_id=w->ordered[w->selected]->word[0x44/4];
        sniper_player_sort(w->ordered,w->count,w->local);
        for(int32_t i=0;i<w->count;++i)
            if(w->ordered[i]->word[0x44/4]==selected_id){w->selected=i;break;}
    }
    return s;
}
void sniper_player_remove(SniperPlayerRegistry *w,int32_t id) {
    w->dirty=1;w->players[id]=NULL;
    for(int32_t i=0;i<w->count;) {
        if(w->ordered[i]->word[0x44/4]!=id){++i;continue;}
        w->release(w->user,w->ordered[i]);
        --w->count;
        memmove(w->ordered+i,w->ordered+i+1,(size_t)(w->count-i)*sizeof *w->ordered);
        if(i<w->selected)--w->selected;
    }
}
