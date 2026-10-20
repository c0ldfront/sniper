#ifndef SNIPER_DEATH_EVENT_H
#define SNIPER_DEATH_EVENT_H
#include "pickup_collect.h"
#include "notices.h"
typedef struct {
    SniperShipObject **players;
    SniperShipObject *local;
    int32_t local_id,kill_bounty,pickup_lifetime;
    SniperPickupWorld *pickups;
    SniperNotices *notices;
    SniperPickupObject *(*allocate)(void *);
    void (*damage)(void *,SniperShipObject *,int32_t,int32_t);
    void (*detach)(void *,int32_t);
    void *user;
} SniperDeathWorld;
/*404680. Valid IDs0..1023; local is required even if either player is missing.
 * Names must be null terminated and fit original31-byte name storage. */
void sniper_death_event(SniperDeathWorld *,int32_t killer,int32_t victim,int32_t points,int32_t prize);
/*411540: offline victim is always player0, prize suppressed. */
void sniper_offline_death(SniperDeathWorld *,int32_t killer,int32_t bounty);
#endif
