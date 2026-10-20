#ifndef SNIPER_SHIP_EFFECT_DRAW_H
#define SNIPER_SHIP_EFFECT_DRAW_H
#include "ship_create.h"
typedef void (*SniperEffectDraw)(void *,int resource,int32_t x,int32_t y,int32_t frame);
/* Original413c20. Draw calls are centered, transparent, in slot order.
 * Outer ship renderer supplies low-detail suppression separately. */
void sniper_ship_effect_draw(const SniperShipObject *,int32_t local_id,
    int32_t camera_x,int32_t camera_y,SniperEffectDraw,void *);
#endif
