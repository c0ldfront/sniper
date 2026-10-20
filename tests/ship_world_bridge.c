#include "ship_world.h"
#include <string.h>
static SniperShipWorld world;
static int32_t *events;
static int varying_clock;
static uint32_t clock_value,clock_stride,clock_calls;
static uint32_t live_clock(void *unused) {
    (void)unused;uint32_t result=clock_value;clock_value+=clock_stride;++clock_calls;return result;
}
void test_ship_world_clock(uint32_t value,uint32_t stride) {
    varying_clock=1;clock_value=value;clock_stride=stride;clock_calls=0;
}
uint32_t test_ship_world_clock_calls(void){return clock_calls;}
static void sound(void *unused,int32_t x,int32_t y) {
    (void)unused;events[0]++;events[1]=x;events[2]=y;
}
static void detach(void *unused,int32_t id) {
    (void)unused;events[3]++;events[4]=id;
    world.players[1]->word[0x6c/4]=id;
}
/* Test-only bridge: input scalars ms,dt,seed,players; output RNG,dirty,events. */
void test_ship_world(SniperShipObject *ship,SniperShipObject *local,
    SniperShipObject *parent,uint8_t *map,uint8_t *coverage,int32_t values[11]) {
    memset(&world,0,sizeof world);sniper_local_config(&world.config);
    world.players[1]=local;world.players[2]=parent;
    world.movement.map=map;world.movement.local_id=1;world.movement.watched_id=2;
    world.movement.impact_sound=sound;
    world.random_state=(uint32_t)values[2];world.milliseconds=(uint32_t)values[0];
    world.player_count=values[3];world.position_dirty=-7;
    world.detach=detach;events=values+6;memset(events,0,5*sizeof *events);
    sniper_ship_world_update(&world,ship,values[1]);
    values[4]=(int32_t)world.random_state;values[5]=world.position_dirty;
    memcpy(coverage,world.coverage,sizeof world.coverage);
}
/* Four-ship frame fixture; preserves object states across caller invocations. */
void test_ship_world_all(SniperShipObject ships[4],const int32_t order[4],
    uint8_t *map,uint8_t *coverage,int32_t values[11]) {
    SniperShipObject *ordered[4];
    memset(&world,0,sizeof world);sniper_local_config(&world.config);
    if(varying_clock)world.clock_ms=live_clock;
    for(int i=0;i<4;++i){world.players[i+(values[3]==0 ? 0:1)]=&ships[i];ordered[i]=&ships[order[i]];}
    world.movement.map=map;world.movement.local_id=1;world.movement.watched_id=2;
    world.movement.impact_sound=sound;
    world.random_state=(uint32_t)values[2];world.milliseconds=(uint32_t)values[0];
    if(values[3]==0){world.offline_backend=1;world.movement.local_id=0;}
    world.player_count=4;world.position_dirty=values[5];world.detach=detach;
    events=values+6;memset(events,0,5*sizeof *events);
    sniper_ship_world_update_all(&world,ordered,&values[1]);
    values[4]=(int32_t)world.random_state;values[5]=world.position_dirty;
    memcpy(coverage,world.coverage,sizeof world.coverage);
}
