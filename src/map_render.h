#ifndef SNIPER87_MAP_RENDER_H
#define SNIPER87_MAP_RENDER_H
#include "map.h"
#include "sprite.h"
typedef void (*SniperDrawTile)(void *context, unsigned tile, int32_t x,
                              int32_t y, int32_t frame, int opaque);
/* Recovered 0x406b70: emit visible tile draws in original order, including
 * the five-cell margin required by special sprites. Dimensions are pixels. */
int sniper_map_draw(const uint8_t map[SNIPER_MAP_BYTES], int32_t camera_x,
                    int32_t camera_y, int32_t width, int32_t height,
                    int32_t tick, SniperDrawTile draw, void *context);
#endif
