#ifndef SNIPER_PICKUP_LIFECYCLE_H
#define SNIPER_PICKUP_LIFECYCLE_H
#include "pickup_collect.h"
/* Caller list has16 slots. Returns1 added,0 rejected,-1 allocation failure.
 * Unlike original's null append, allocation failure leaves list unchanged. */
int sniper_pickup_create(SniperPickupWorld *,int32_t x,int32_t y,int32_t code,
    int32_t lifetime,SniperPickupObject *(*allocate)(void *),void *owner);
/* Main frame4021d5..402237: age,expire,clear tile,release,compact in order. */
void sniper_pickup_expire(SniperPickupWorld *,int32_t ticks);
#endif
