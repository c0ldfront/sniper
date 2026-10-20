#ifndef SNIPER_ACTOR_LAYERS_H
#define SNIPER_ACTOR_LAYERS_H
#include "pickup_collect.h"
#include "projectile_update.h"
typedef struct {
    SniperPickupObject **pickups;const int32_t *pickup_count;
    SniperProjectile **projectiles;const int32_t *projectile_count;
    const int32_t *camera_x,*camera_y;
    void (*pickup)(void *,SniperPickupObject *,int32_t,int32_t);
    void (*projectile)(void *,SniperProjectile *,int32_t,int32_t);
    void (*ships)(void *);
    void *user;
} SniperActorLayers;
/*4024f7..402598,ordinary scene. Counts,camera and impact fields read live.
 * Special-mode bypass,map/background,HUD and overlays handled by caller. */
void sniper_actor_layers(SniperActorLayers *);
#endif
