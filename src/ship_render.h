#ifndef SNIPER_SHIP_RENDER_H
#define SNIPER_SHIP_RENDER_H
#include "ship_visual.h"
#include "ship_labels.h"
/* Full413cb0 draw decisions. Parent is resolved from attachment ID by caller.
 * Sprite callbacks consume atlas selections; text callback consumes labels. */
void sniper_ship_render(SniperShipObject *,const SniperShipObject *local,
    SniperShipObject *parent,uint32_t *blink,int low_detail,int label_mode,
    int32_t upper,int32_t lower,int32_t bonus,int32_t camera_x,int32_t camera_y,
    SniperEffectDraw,SniperBodyDraw,SniperLabelDraw,void *);
#endif
