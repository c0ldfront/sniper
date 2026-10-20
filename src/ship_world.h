#ifndef SNIPER_SHIP_WORLD_H
#define SNIPER_SHIP_WORLD_H
#include "ship_update.h"
#include "ship_spawn.h"
#include "movement.h"
#include "local_config.h"
#include "prize.h"
#include "attachment.h"
#include "ship_update_order.h"
/* Shared state for native update/spawn/movement. Caller owns ships and map.
 * Network detach and pickup application remain backend callbacks. */
typedef struct {
    SniperLocalConfig config;
    /* Oversized portable storage; native valid IDs are0..254. This size
     * does not authorize IDs255..1023 (see player-capacity-findings.txt). */
    SniperShipObject *players[1024];
    SniperMoveWorld movement;
    uint8_t coverage[256*32];
    uint32_t random_state, milliseconds;
    int32_t player_count, position_dirty;
    void (*detach)(void *,int32_t);
    void *backend;
    int offline_backend;
    SniperAttachmentHud attachment_hud;
    int32_t positive_sound,negative_sound,warp_sound;
    void (*prize_sound)(void *,int32_t handle);
    void (*prize_message)(void *,const char *,int32_t duration,int32_t color);
    /* Optional live clock; NULL retains explicit deterministic milliseconds. */
    uint32_t (*clock_ms)(void *);
    void *clock_user;
} SniperShipWorld;
static inline uint32_t sniper_ship_world_clock(SniperShipWorld *w) {
    return w->clock_ms?w->clock_ms(w->clock_user):w->milliseconds;
}
void sniper_ship_world_update(SniperShipWorld *,SniperShipObject *,int32_t ticks);
int sniper_ship_world_prize(SniperShipWorld *,SniperShipObject *,int32_t code);
/* Dense order is supplied separately from players[],which is indexed by ID. */
void sniper_ship_world_update_all(SniperShipWorld *,SniperShipObject **ordered,
    const int32_t *ticks);
void sniper_ship_world_offline_attach(SniperShipWorld *,int32_t target);
#endif
