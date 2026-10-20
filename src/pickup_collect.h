#ifndef SNIPER_PICKUP_COLLECT_H
#define SNIPER_PICKUP_COLLECT_H
#include "ship_create.h"
typedef struct { int32_t age,x,y,lifetime,value; } SniperPickupObject;
typedef struct {
    SniperPickupObject **items;
    int32_t count;
    uint8_t *map;
    SniperShipObject *local;
    int32_t sharing_enabled, sharing_limit;
    SniperShipObject *(*lookup)(void *,int32_t);
    void (*release)(void *,SniperPickupObject *);
    void (*award)(void *,SniperShipObject *,int32_t code);
    void *user;
} SniperPickupWorld;
/* 404910, including412ae0 map clearing. Valid pickup coordinates and non-null
 * item/local pointers required. release disposes storage, not the list itself.
 * Award effects415030 are synchronous but implemented separately. */
void sniper_pickup_collect(SniperPickupWorld *,int32_t x,int32_t y,
    int32_t collector,int32_t code);
#endif
