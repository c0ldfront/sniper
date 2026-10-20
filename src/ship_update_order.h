#ifndef SNIPER_SHIP_UPDATE_ORDER_H
#define SNIPER_SHIP_UPDATE_ORDER_H
#include "ship_create.h"
typedef void (*SniperOrderedUpdate)(void *,SniperShipObject *,int32_t);
/*402237..40229b: dense insertion-ordered player list,NOT ID-indexed table.
 * First unattached,then attached; membership,count and ticks are read live.
 * Caller guarantees non-null entries and capacity for synchronous additions. */
void sniper_ship_update_order(SniperShipObject **ordered,const int32_t *count,
    const int32_t *ticks,SniperOrderedUpdate,void *user);
#endif
