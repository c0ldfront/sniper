#ifndef SNIPER_SHIP_VISUAL_H
#define SNIPER_SHIP_VISUAL_H
#include "ship_body_draw.h"
#include "ship_effect_draw.h"
/* Shared blink counter is original global433850, not a per-ship counter.
 * Returns1 when visual body path executes,0 when rejected. Labels remain
 * a separate stage, executed only when this returns1. */
int sniper_ship_visual(SniperShipObject *,uint32_t *blink_counter,int low_detail,
    int32_t local_id,int local_field94,int32_t camera_x,int32_t camera_y,
    SniperEffectDraw,SniperBodyDraw,void *);
#endif
