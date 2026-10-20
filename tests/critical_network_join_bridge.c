/* Persistent join/leave/rejoin, team and death using production registry. */
#include "critical_network_death_bridge.c"
static SniperShipObject join_pool[4];static int join_allocated;
static SniperShipObject *join_allocate(void*u){(void)u;if(join_allocated>=4)abort();effect(7,join_allocated,0,0);return &join_pool[join_allocated++];}
static void join_release(void*u,SniperShipObject*p){(void)u;effect(8,(int)(p-join_pool),0,0);}
void critical_join_init(uint32_t(*clock)(void*),int32_t(*read)(void*,uint8_t*,int32_t,int),int32_t(*send)(void*,const uint8_t*,int32_t),void(*trace)(const int32_t*,const void*),const void*initial,const uint8_t*map,uint32_t offset,int unused){
 critical_death_init(clock,read,send,trace,initial,map,offset,unused);memset(join_pool,0x55,sizeof join_pool);join_allocated=0;
 players[7]=NULL;ordered[0]=players[42];ordered[1]=NULL;registry.count=1;registry.selected=0;registry.dirty=0;registry.notices=&session->ships.attachment_hud;registry.defaults=&defaults;registry.allocate=join_allocate;registry.release=join_release;registry.clock_ms=clock_ms;
 team.detach=death_detach;backend.team=&team;
}
void critical_join_snapshot(int which,void*out){
 if(which==0)memcpy(out,join_pool,sizeof join_pool);
 if(which==1){int32_t v[]={join_allocated,registry.count,registry.selected,registry.dirty,players[7]?(int32_t)(players[7]-join_pool):-1,ordered[0]?ordered[0]->word[0x44/4]:-1,registry.count>1?ordered[1]->word[0x44/4]:-1};memcpy(out,v,sizeof v);}
}
