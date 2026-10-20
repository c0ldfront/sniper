#ifndef SNIPER_PROJECTILE_FRAME_H
#define SNIPER_PROJECTILE_FRAME_H
#include "projectile_update.h"
typedef int (*SniperProjectileStep)(void *,SniperProjectile *,int32_t);
typedef void (*SniperProjectileRelease)(void *,SniperProjectile *);
/*40229b..4022f2. Reads list/count/ticks live; synchronous appended shrapnel is
 * visited this frame. Release precedes decrement/compaction. Stale tail kept.
 * Caller maintains valid non-null entries and capacity during callbacks. */
void sniper_projectile_frame(SniperProjectile **,int32_t *count,const int32_t *ticks,
    SniperProjectileStep,SniperProjectileRelease,void *);
/* Connected update/frame execution. Explosion callbacks that append must
 * update this same world list/count so additions participate immediately. */
void sniper_projectile_world_frame(SniperProjectileWorld *,const int32_t *ticks,
    SniperProjectileRelease,void *);
#endif
