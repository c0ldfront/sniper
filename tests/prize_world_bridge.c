#include "ship_world.h"
#include <string.h>
static SniperShipWorld world;
void test_prize_begin(SniperShipObject *ship,uint8_t *map,uint32_t seed,
    void (*sound)(void *,int32_t),void (*message)(void *,const char *,int32_t,int32_t)) {
    memset(&world,0,sizeof world);sniper_local_config(&world.config);
    world.config.word[(0x428289-0x4281c9)/4]=250;
    world.config.word[(0x42828d-0x4281c9)/4]=250;
    world.players[1]=ship;world.movement.map=map;world.movement.local_id=1;
    world.movement.watched_id=1;world.random_state=seed;world.milliseconds=10000;
    world.player_count=1;world.position_dirty=-7;
    world.positive_sound=101;world.negative_sound=102;world.warp_sound=103;
    world.prize_sound=sound;world.prize_message=message;
}
void test_prize_action(SniperShipObject *ship,int prize,int32_t value,
    uint32_t result[2],uint8_t *coverage) {
    if(prize)sniper_ship_world_prize(&world,ship,value);
    else sniper_ship_world_update(&world,ship,value);
    result[0]=world.random_state;result[1]=(uint32_t)world.position_dirty;
    memcpy(coverage,world.coverage,sizeof world.coverage);
}
