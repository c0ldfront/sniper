#ifndef SNIPER_PROJECTILE_EXPLOSION_H
#define SNIPER_PROJECTILE_EXPLOSION_H
#include "projectile_update.h"
typedef struct {
    SniperShipObject *local;
    int32_t radius_per_level,maximum_damage,shake_duration,shake;
    int32_t shrapnel_speed,next_group,projectile_count;
    SniperProjectile **projectiles; /* room for 512 entries */
    void (*sound)(void *,int32_t,int32_t);
    void (*damage)(void *,SniperShipObject *,int32_t,int32_t);
    /* Allocate and construct a 72-byte projectile from the 15 original args.
       Return NULL on failure. Original appends NULL too; retained here. */
    SniperProjectile *(*create)(void *,const int32_t args[15]);
    void *user;
} SniperExplosionWorld;
void sniper_projectile_explode(SniperProjectile *,SniperExplosionWorld *);
#endif
