#ifndef SNIPER_CAMERA_H
#define SNIPER_CAMERA_H
#include "ship_create.h"
typedef struct { int32_t x,y; } SniperCamera;
/*402338..402468. Special mode follows tile coordinates and bypasses shake.
 * Otherwise type4 follows watched ship (NULL means origin),others local ship.
 * Extents are42818c/42434c. Shared CRT RNG advances twice only for shake>0. */
SniperCamera sniper_camera(const SniperShipObject *local,const SniperShipObject *watched,
    int32_t width,int32_t height,int special,int32_t tile_x,int32_t tile_y,
    int32_t shake,uint32_t *random_state);
#endif
