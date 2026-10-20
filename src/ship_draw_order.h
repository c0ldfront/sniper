#ifndef SNIPER_SHIP_DRAW_ORDER_H
#define SNIPER_SHIP_DRAW_ORDER_H
#include "ship_create.h"
typedef void (*SniperOrderedShipDraw)(void *,SniperShipObject *,int32_t,int32_t);
typedef struct {
    SniperShipObject **ships,*local;
    int32_t count,local_id,camera_x,camera_y;
    uint32_t frame;
    SniperOrderedShipDraw draw;
    void *user;
} SniperShipDrawOrder;
/*406d00. Leader flag84 refreshed every16 invocations,first tied score wins.
 * Empty list safely skips leader selection (original indexes list[-1]). */
void sniper_ship_draw_order(SniperShipDrawOrder *);
#endif
