#ifndef SNIPER_PROJECTILE_UPDATE_H
#define SNIPER_PROJECTILE_UPDATE_H
#include "ship_create.h"
typedef struct { int32_t word[18]; } SniperProjectile;
typedef struct {
    const uint8_t *map,*coverage;
    int32_t coverage_stride;
    SniperShipObject **ships;int32_t ship_count;
    SniperProjectile **projectiles;int32_t projectile_count;
    int32_t local_id,bullet_lifetime,bomb_lifetime,proximity_bonus,proximity_delay,damage_divisor;
    void (*explode)(void *,SniperProjectile *);
    void (*sound)(void *,int32_t x,int32_t y);
    int32_t (*random)(void *);
    void (*damage)(void *,SniperShipObject *,int32_t amount,int32_t owner);
    void *user;
} SniperProjectileWorld;
/*413010. Returns1 when caller should release object. Valid map/coverage and
 * positive damage divisor required; RNG must return CRT range0..32767. */
int sniper_projectile_update(SniperProjectile *,SniperProjectileWorld *,int32_t ticks);
#endif
