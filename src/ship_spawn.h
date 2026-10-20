#ifndef SNIPER_SHIP_SPAWN_H
#define SNIPER_SHIP_SPAWN_H
#include "ship_reset.h"
/* Original MS CRT random sequence at0x4165f0; state is shared by game systems. */
uint32_t sniper_random(uint32_t *state);
/* Full0x4137c0 lifecycle. Valid player count0..1024 and map with an accessible
 * empty3x3 area required. Like original, occupied maps can retry indefinitely.
 * move/reset correspond to second/first original stack arguments. */
void sniper_ship_spawn(int32_t object[0x110/4],const SniperShipDefaults *,
    const uint8_t *map,int32_t player_count,uint32_t milliseconds,uint32_t *random_state,
    int reset,int move,int32_t *position_dirty);
#endif
