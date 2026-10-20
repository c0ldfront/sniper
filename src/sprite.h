#ifndef SNIPER87_SPRITE_H
#define SNIPER87_SPRITE_H
#include <stdint.h>
typedef struct {
    int32_t resource, origin_x, origin_y, width, height;
    int32_t rows, columns, period, frames;
} SniperSprite;
typedef struct { int32_t left, top, right, bottom; } SniperRect;
typedef struct { SniperRect source, destination; uint32_t flags; } SniperBlit;
extern const SniperSprite sniper_large_tiles[4];
/* 0x406c71..0x406c96: signed tick calculation, positive period/frames. */
int sniper_sprite_frame(const SniperSprite *sprite, int32_t tick, int32_t *frame);
/* 0x4105c0..0x41069d: DirectDraw Blt geometry and source color-key flag.
 * Source origin fields are used by resource loading, not this draw routine. */
int sniper_sprite_blit(const SniperSprite *sprite, int32_t x, int32_t y,
                       int32_t frame, int centered, int opaque, SniperBlit *out);
#endif
