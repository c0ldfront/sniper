#include "manual_warp.h"
#include "notices.h"
int sniper_manual_warp(SniperShipWorld *world,SniperShipObject *ship) {
    if(ship->word[0x5c/4]!=ship->word[0xc8/4]) {
        sniper_notice_push(&world->attachment_hud,"Not enough energy to warp.",300,2);
        return 0;
    }
    if(world->prize_sound)world->prize_sound(world->backend,world->warp_sound);
    ship->word[0x5c/4]=0;
    SniperShipDefaults defaults;sniper_local_ship_defaults(&world->config,&defaults);
    sniper_ship_spawn(ship->word,&defaults,world->movement.map,world->player_count,
        sniper_ship_world_clock(world),&world->random_state,0,1,&world->position_dirty);
    return 1;
}
