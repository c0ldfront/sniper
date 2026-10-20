#include "offline_pickups.h"
#include <string.h>
static SniperShipWorld world;
static SniperOfflinePickups pickups;
static SniperPickupObject *items[16];
static SniperPickup view[16];
void test_pickup_sequence_begin(uint8_t *map,uint32_t seed,uint32_t previous,
    SniperPickupObject *(*allocate)(void *),void (*release)(void *,SniperPickupObject *),
    uint32_t (*clock_ms)(void *)) {
    memset(&world,0,sizeof world);memset(&pickups,0,sizeof pickups);
    world.movement.map=map;world.random_state=seed;
    pickups.ships=&world;pickups.collection.items=items;pickups.movement_view=view;
    pickups.allocate=allocate;pickups.release=release;pickups.clock_ms=clock_ms;
    pickups.generation_tick=previous;sniper_offline_pickups_bind(&pickups);
}
void test_pickup_sequence_step(int generate,int32_t ticks,uint32_t result[4],
    SniperPickupObject output[16],SniperPickup movement[16]) {
    if(generate)sniper_offline_pickups_generate(&pickups);
    else sniper_offline_pickups_expire(&pickups,ticks);
    result[0]=world.random_state;result[1]=pickups.generation_tick;
    result[2]=(uint32_t)pickups.collection.count;
    result[3]=(uint32_t)world.movement.pickup_count;
    for(int32_t i=0;i<pickups.collection.count;++i) {
        output[i]=*items[i];movement[i]=world.movement.pickups[i];
    }
}
