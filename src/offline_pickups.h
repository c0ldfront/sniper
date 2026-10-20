#ifndef SNIPER_OFFLINE_PICKUPS_H
#define SNIPER_OFFLINE_PICKUPS_H
#include "ship_world.h"
#include "pickup_collect.h"
#include "pickup_lifecycle.h"
#include "offline_generation.h"
typedef struct {
    SniperShipWorld *ships;
    SniperPickupWorld collection;
    SniperPickup *movement_view; /* caller storage, capacity >=16 */
    void (*release)(void *,SniperPickupObject *);
    void *owner;
    SniperPickupObject *(*allocate)(void *);
    uint32_t generation_tick;
    uint32_t (*clock_ms)(void *);
    void *clock_user;
    void (*impact_sound)(void *,int32_t,int32_t);
    void *impact_user;
} SniperOfflinePickups;
/* Offline vtable420008+1c ->411560 passes literal collector0.
 * Bind captures existing movement sound/user before replacing movement hooks.
 * Set collection.items/count,ships,movement_view,release/owner before binding.
 * Offline world must contain its local player at ID0. */
void sniper_offline_pickups_bind(SniperOfflinePickups *);
void sniper_offline_pickups_collect(SniperOfflinePickups *,int32_t x,int32_t y,
    int32_t collector,int32_t code);
void sniper_offline_pickups_generate(SniperOfflinePickups *);
void sniper_offline_pickups_expire(SniperOfflinePickups *,int32_t ticks);
#endif
