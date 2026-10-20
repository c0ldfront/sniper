#include "offline_pickups.h"
#include <string.h>
static SniperShipWorld world;
static SniperOfflinePickups pickups;
static SniperPickupObject *items[32];
static SniperPickup view[32];
static int32_t freed;
static void release(void *unused,SniperPickupObject *item) {
    (void)unused;freed=freed*31+item->age+1;
}
void test_offline_pickups(SniperShipObject *ship,SniperPickupObject *objects,
    int32_t count,uint8_t *map,int32_t ticks,uint32_t seed,
    void (*sound)(void *,int32_t),void (*message)(void *,const char *,int32_t,int32_t),
    uint32_t result[36],uint8_t *coverage) {
    memset(&world,0,sizeof world);memset(&pickups,0,sizeof pickups);freed=0;
    sniper_local_config(&world.config);
    world.players[0]=ship;world.movement.map=map;world.random_state=seed;
    world.milliseconds=10000;world.player_count=1;world.position_dirty=-7;
    world.positive_sound=101;world.negative_sound=102;world.warp_sound=103;
    world.prize_sound=sound;world.prize_message=message;
    for(int32_t i=0;i<count;++i)items[i]=objects+i;
    pickups.ships=&world;pickups.collection.items=items;pickups.collection.count=count;
    pickups.movement_view=view;pickups.release=release;
    sniper_offline_pickups_bind(&pickups);
    sniper_ship_world_update(&world,ship,ticks);
    result[0]=world.random_state;result[1]=(uint32_t)world.position_dirty;
    result[2]=(uint32_t)pickups.collection.count;result[3]=(uint32_t)freed;
    for(int32_t i=0;i<pickups.collection.count;++i)result[4+i]=(uint32_t)items[i]->age;
    memcpy(coverage,world.coverage,sizeof world.coverage);
}
