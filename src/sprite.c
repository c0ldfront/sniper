#include "sprite.h"
#include <limits.h>
#include <stddef.h>
/* Nine-DWORD records 0x421910,0x421934,0x421958,0x42197c. */
const SniperSprite sniper_large_tiles[4] = {
    {154,0,0,16,16,2,15,150,30},
    {155,0,0,32,32,3,10,150,30},
    {156,0,0,64,64,6,5,240,30},
    {157,0,0,96,96,5,3,120,15}
};
int sniper_sprite_frame(const SniperSprite *s, int32_t tick, int32_t *frame) {
    if (!s || !frame || s->period <= 0 || s->frames <= 0 ||
        s->period / s->frames == 0) return -1;
    *frame = (tick % s->period) / (s->period / s->frames);
    return 0;
}
static int rect(SniperRect *r, int64_t x, int64_t y, int32_t w, int32_t h) {
    if (x < INT32_MIN || y < INT32_MIN || x+w > INT32_MAX || y+h > INT32_MAX)
        return -1;
    *r = (SniperRect){(int32_t)x,(int32_t)y,(int32_t)(x+w),(int32_t)(y+h)};
    return 0;
}
int sniper_sprite_blit(const SniperSprite *s, int32_t x, int32_t y,
                       int32_t frame, int centered, int opaque, SniperBlit *out) {
    if (!s || !out || s->width <= 0 || s->height <= 0 ||
        s->rows <= 0 || s->columns <= 0) return -1;
    SniperBlit b;
    int64_t dx=x,dy=y,sx,sy;
    if (centered) { dx-=s->width/2; dy-=s->height/2; }
    if (s->rows==1) { sx=(int64_t)frame*s->width; sy=0; }
    else { sx=(int64_t)(frame%s->columns)*s->width;
           sy=(int64_t)(frame/s->columns)*s->height; }
    if (rect(&b.source,sx,sy,s->width,s->height) ||
        rect(&b.destination,dx,dy,s->width,s->height)) return -1;
    b.flags=opaque ? 0 : 0x8000;
    *out=b;
    return 0;
}
