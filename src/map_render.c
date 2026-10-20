#include "map_render.h"
int sniper_map_draw(const uint8_t map[SNIPER_MAP_BYTES], int32_t cx,
                    int32_t cy, int32_t width, int32_t height,
                    int32_t tick, SniperDrawTile draw, void *context) {
    /* Limit to the actual finite map/view domain to avoid signed overflow. */
    if (!map || !draw || cx < -32768 || cx > 32768 || cy < -32768 ||
        cy > 32768 || width <= 0 || width > 32768 || height <= 0 || height > 32768)
        return -1;
    int left=(cx-15)/16, top=(cy-15)/16;
    for (int y=top-5; y*16-cy < height; ++y) {
        if (y < -1 || y > 1024) continue;
        for (int x=left-5; x*16-cx < width; ++x) {
            if (x < -1 || x > 1024) continue;
            unsigned tile=(x==-1 || y==-1 || x==1024 || y==1024)
                ? 20 : map[y*1024+x];
            if (tile==250) continue;
            if (tile>=216 && tile<250) {
                if (tile>219) return -1; /* Original dereferences absent descriptor. */
                int32_t frame;
                if (sniper_sprite_frame(&sniper_large_tiles[tile-216],tick,&frame)) return -1;
                draw(context,tile,x*16-cx,y*16-cy,frame,0);
            } else if (tile>=1 && tile<153 && x>=left && y>=top) {
                draw(context,tile,x*16-cx,y*16-cy,(int32_t)tile-1,1);
            }
        }
    }
    return 0;
}
