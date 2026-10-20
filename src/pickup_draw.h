#ifndef SNIPER_PICKUP_DRAW_H
#define SNIPER_PICKUP_DRAW_H
#include "pickup_collect.h"
#include "sprite.h"
extern const SniperSprite sniper_pickup_sprite;
typedef void (*SniperPickupDraw)(void *,int32_t x,int32_t y,int32_t frame,int centered,int opaque);
/* Original412b20; caller owns surface142. Does not perform viewport culling. */
void sniper_pickup_draw(const SniperPickupObject *,int32_t camera_x,int32_t camera_y,
    SniperPickupDraw,void *);
#endif
