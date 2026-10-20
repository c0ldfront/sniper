#include "offline_simulation.h"
#include <string.h>
static int32_t sub(int32_t a,int32_t b){uint32_t u=(uint32_t)a-(uint32_t)b;int32_t s;memcpy(&s,&u,4);return s;}
void sniper_offline_simulation(SniperOfflineCombat *c,SniperOfflinePickups *pickups,const int32_t *ticks){
    memset(c->ships->coverage,0,sizeof c->ships->coverage);
    if(pickups)sniper_offline_pickups_expire(pickups,*ticks);
    sniper_ship_world_update_all(c->ships,c->ordered_ships,ticks);
    sniper_offline_combat_frame(c,ticks);
    sniper_notices_step(&c->ships->attachment_hud,*ticks);
    if(c->shake>0)c->shake=sub(c->shake,*ticks);
    if(c->repel_timer>0)c->repel_timer=sub(c->repel_timer,*ticks);
}
