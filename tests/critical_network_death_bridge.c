/* Add actual inbound death world and outbound detach to the stable network fixture. */
#include "critical_network_bridge.c"
static SniperDeathWorld death_world;
static SniperPickupObject death_items[16];
static SniperPickupObject *death_list[16];
static SniperPickupWorld death_pickups;
static int death_allocated;
static SniperPickupObject*death_allocate(void*u){(void)u;if(death_allocated>=16)abort();effect(6,death_allocated,0,0);return &death_items[death_allocated++];}
static void death_damage(void*u,SniperShipObject*p,int32_t amount,int32_t owner){(void)u;sniper_online_gameplay_damage(&gameplay,p,amount,owner);}
static void death_detach(void*u,int32_t target){(void)u;sniper_network_client_attach(&client,target);}
void critical_death_init(uint32_t(*clock)(void*),int32_t(*read)(void*,uint8_t*,int32_t,int),int32_t(*send)(void*,const uint8_t*,int32_t),void(*trace)(const int32_t*,const void*),const void*initial,const uint8_t*map,uint32_t offset,int unused){
 (void)unused;critical_network_init(clock,read,send,trace,initial,map,offset,2);
 memset(death_items,0x55,sizeof death_items);memset(death_list,0,sizeof death_list);death_allocated=0;
 death_pickups=(SniperPickupWorld){0};death_pickups.items=death_list;death_pickups.map=session->map;death_pickups.local=players[42];
 death_world=(SniperDeathWorld){players,players[42],42,7,6000,&death_pickups,&session->ships.attachment_hud,death_allocate,death_damage,death_detach,NULL};
 backend.death=&death_world;backend.local_id=&client.game.player_id;backend.attachment_hud=&session->ships.attachment_hud;
}
void critical_death_snapshot(int which,void*out){
 if(which==0)memcpy(out,death_items,sizeof death_items);
 if(which==1){int32_t v[]={death_allocated,death_pickups.count};memcpy(out,v,sizeof v);}
 if(which==2)memcpy(out,session->map,1024*1024);
 if(which==3)memcpy(out,&session->ships.attachment_hud,sizeof session->ships.attachment_hud);
 if(which==4)for(int i=0;i<16;i++)((int32_t*)out)[i]=death_list[i]?(int32_t)(death_list[i]-death_items):-1;
}
