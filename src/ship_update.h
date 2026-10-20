#ifndef SNIPER_SHIP_UPDATE_H
#define SNIPER_SHIP_UPDATE_H
#include "ship_create.h"
typedef struct {
    int32_t local_id, respawn_delay, remote_energy;
    int32_t jitter_period, jitter_radius, ability_cost[4];
    uint32_t milliseconds;
    int32_t local_team;
    uint8_t *coverage; /* 256 rows, at least 32 bytes per row */
    int32_t coverage_stride;
    SniperShipObject *(*lookup)(void *, int32_t id);
    void (*detach)(void *, int32_t id);
    void (*spawn)(void *, SniperShipObject *, int reset, int move);
    void (*move)(void *, SniperShipObject *, int32_t ticks, int effects);
    int32_t (*random)(void *);
    void *user;
} SniperShipUpdateWorld;
/* Full413fa0 orchestration. Required callbacks run synchronously and may mutate
 * ships/world. Random returns original CRT range0..32767; jitter divisors >0.
 * Timestamp supplied by caller represents GetTickCount at remote-age check. */
void sniper_ship_update(SniperShipObject *, SniperShipUpdateWorld *, int32_t ticks);
#endif
