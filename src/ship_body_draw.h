#ifndef SNIPER_SHIP_BODY_DRAW_H
#define SNIPER_SHIP_BODY_DRAW_H
#include "ship_create.h"
enum { SNIPER_BODY_TYPE0, SNIPER_BODY_TYPE1, SNIPER_BODY_TYPE2, SNIPER_BODY_TYPE3,
       SNIPER_BODY_OVERRIDE, SNIPER_BODY_DEATH, SNIPER_BODY_SPAWN,
       SNIPER_BODY_ATTACHED_LOCAL, SNIPER_BODY_ATTACHED_REMOTE };
typedef void (*SniperBodyDraw)(void *,int slot,int32_t x,int32_t y,int32_t frame);
/* Original413d1d..413e10 only. Caller handles visibility, particles and labels.
 * type must0..3 when normal ship sprite is selected. Draws centered/transparent. */
void sniper_ship_body_draw(SniperShipObject *,int32_t local_id,int local_field94,
    int32_t camera_x,int32_t camera_y,SniperBodyDraw,void *);
#endif
