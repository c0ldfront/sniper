#ifndef SNIPER_STARFIELD_H
#define SNIPER_STARFIELD_H
#include <stdint.h>
#define SNIPER_STAR_COUNT (63*63)
typedef struct { int16_t x,y; } SniperStar;
typedef struct { SniperStar near_layer[SNIPER_STAR_COUNT],far_layer[SNIPER_STAR_COUNT]; } SniperStarfield;
/* 403569..403612: consumes four CRT random values per cell, shared RNG. */
void sniper_starfield_generate(SniperStarfield *,uint32_t *rng);
/* 409840: indexed pixels, no clear. Caller supplies the locked surface and
 * palette indices. Returns -1 for invalid dimensions/pitch or table range
 * (original would access outside its tables); otherwise zero. */
int sniper_starfield_draw(const SniperStarfield *,uint8_t *pixels,int32_t pitch,
    int32_t width,int32_t height,int32_t camera_x,int32_t camera_y,
    uint8_t near_color,uint8_t far_color);
#endif
