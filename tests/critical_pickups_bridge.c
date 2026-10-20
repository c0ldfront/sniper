#include "offline_pickups.h"
#include "notices.h"
#include "weapon_action.h"
#include "ship_flight.h"
#include "numeric_environment.h"
#include <string.h>
#include <assert.h>
static SniperShipWorld world;
static SniperOfflinePickups pickups;
static SniperPickupObject pool[16],*items[16];
static SniperPickup view[16];
static int used[16];
static int32_t exhaust_ticks;
static SniperWeaponActionWorld weapons;
static void (*allocation_event)(int,int);
static SniperPickupObject *allocate(void *unused){
    (void)unused;for(int i=0;i<16;i++)if(!used[i]){used[i]=1;allocation_event(1,i);return pool+i;}assert(0);return 0;
}
static void release(void *unused,SniperPickupObject *p){
    (void)unused;int i=(int)(p-pool);assert(used[i]);used[i]=0;allocation_event(0,i);
}
void critical_pickups_begin(SniperShipObject *ship,uint8_t *map,uint32_t seed,
    void (*sound)(void *,int32_t),void (*message)(void *,const char *,int32_t,int32_t),
    void (*event)(int,int),void (*weapon)(void *,const int32_t *)){
    unsigned short control=0x037f;__asm__ volatile("fldcw %0"::"m"(control));
    sniper_numeric_initialize();__asm__ volatile("fnstcw %0":"=m"(control));assert(control==0x027f);
    memset(&world,0,sizeof world);memset(&pickups,0,sizeof pickups);memset(used,0,sizeof used);
    exhaust_ticks=0;memset(pool,0,sizeof pool);memset(&weapons,0,sizeof weapons);allocation_event=event;
    sniper_local_config(&world.config);world.config.word[(0x428289-0x4281c9)/4]=250;
    world.config.word[(0x42828d-0x4281c9)/4]=250;
    world.players[0]=ship;world.movement.map=map;world.random_state=seed;
    world.milliseconds=10000;world.player_count=1;world.position_dirty=-7;
    world.positive_sound=101;world.negative_sound=102;world.warp_sound=103;
    world.prize_sound=sound;world.prize_message=message;
    pickups.ships=&world;pickups.collection.items=items;pickups.collection.map=map;
    pickups.movement_view=view;pickups.release=release;
    sniper_offline_pickups_bind(&pickups);
    weapons.local=ship;weapons.config=&world.config;weapons.map=map;
    weapons.notices=&world.attachment_hud;weapons.weapon=weapon;
}
void critical_pickups_add(int32_t x,int32_t y,int32_t code,int32_t life){
    sniper_pickup_create(&pickups.collection,x,y,code,life,allocate,0);
    /* Expire0 maintains exactly the production movement view after creation. */
    sniper_offline_pickups_expire(&pickups,0);
}
void critical_pickups_frame(int32_t ticks){
    sniper_ship_world_update(&world,world.players[0],ticks);
    sniper_offline_pickups_expire(&pickups,ticks);
    sniper_notices_step(&world.attachment_hud,ticks);
    weapons.cooldown-=ticks;weapons.safe_timer-=ticks;
}
void critical_pickups_fire(int32_t action,int32_t fields[6]){
    weapons.position_dirty=world.position_dirty;
    sniper_weapon_action_fields(&weapons,1,action,0,fields);
    world.position_dirty=weapons.position_dirty;
}
void critical_pickups_flight(uint32_t keys,int32_t ticks,int32_t result[2]){
    SniperFlightInput input={.keys=keys};SniperBoostConfig boost;
    SniperFlightEffects effects;int32_t scratch=0;sniper_local_boost(&world.config,&boost);
    sniper_ship_flight(world.players[0],&input,0,&boost,ticks,1,1,&exhaust_ticks,&effects,&scratch);
    for(int i=0;i<effects.sound_count;i++)world.prize_sound(0,200+effects.sound_slot[i]*2+effects.sound_play[i]);
    result[0]=exhaust_ticks;result[1]=scratch;
}
void critical_pickups_snapshot(uint32_t result[5],SniperPickupObject output[16],
    uint8_t coverage[8192],SniperNotices *hud){
    result[0]=world.random_state;result[1]=(uint32_t)world.position_dirty;
    result[2]=(uint32_t)pickups.collection.count;
    result[3]=(uint32_t)weapons.cooldown;result[4]=(uint32_t)weapons.safe_timer;
    for(int i=0;i<pickups.collection.count;i++)output[i]=*items[i];
    memcpy(coverage,world.coverage,8192);*hud=world.attachment_hud;
}
