#ifndef SNIPER_POSITION_DISPATCH_H
#define SNIPER_POSITION_DISPATCH_H
#include "ship_create.h"
typedef struct {
    SniperShipObject *local;
    int32_t interval,ticks,last_ticks,last_vx,last_vy,last_heading,position_dirty;
    void (*send)(void *,const int32_t args[13]);
    void *user;
} SniperPositionDispatch;
/*406922..406a84. weapon fields:type,level,fragmentType,fragmentLevel,
 * fragmentCount,multifire. Backend receives13 unencoded integers.
 * Returns1 if dispatched. Does not perform packet encoding or networking. */
int sniper_position_dispatch(SniperPositionDispatch *,const int32_t weapon[6]);
#endif
