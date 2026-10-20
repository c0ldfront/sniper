#ifndef SNIPER_PROJECTILE_CREATE_H
#define SNIPER_PROJECTILE_CREATE_H
#include "projectile_update.h"
typedef struct {
    SniperShipObject *local;
    const uint8_t *map;
    SniperProjectile **projectiles;
    int32_t projectile_count,repel_range,repel_speed,repel_duration,repel_timer;
    int32_t bullet_sounds[4],bomb_sounds[4],repel_sound;
    void (*sound)(void *,int32_t handle,int32_t x,int32_t y);
    void *user;
} SniperProjectileCreateWorld;
/*412b60. args: owner,x,y,vx,vy,heading,speed,type,level,fragmentType,
 * fragmentLevel,fragmentCount,group,team,grace. Valid sound level0..3.
 * Preserves uninitialized previous-position fields28/2c. x87 required. */
void sniper_projectile_create(SniperProjectile *,SniperProjectileCreateWorld *,const int32_t args[15]);
#endif
