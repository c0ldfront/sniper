#include "map.h"
static int footprint(const SniperTileSize sizes[256], unsigned tile,
                     unsigned x, unsigned y, unsigned *w, unsigned *h) {
    *w = *h = 0;
    if (tile < 216) return 0;
    if (sizes[tile].width <= 0 || sizes[tile].height <= 0) return -1;
    *w = (unsigned)(sizes[tile].width / 16);
    *h = (unsigned)(sizes[tile].height / 16);
    return *w > SNIPER_MAP_SIDE-x || *h > SNIPER_MAP_SIDE-y ? -1 : 0;
}
int sniper_map_place(uint8_t map[SNIPER_MAP_BYTES],
                     const SniperTileSize sizes[256],
                     unsigned x, unsigned y, uint8_t tile) {
    if (!map || !sizes || x >= SNIPER_MAP_SIDE || y >= SNIPER_MAP_SIDE) return -1;
    unsigned ow,oh,nw,nh;
    if (footprint(sizes,map[y*SNIPER_MAP_SIDE+x],x,y,&ow,&oh) ||
        footprint(sizes,tile,x,y,&nw,&nh)) return -1;
    for (unsigned dx=0; dx<ow; ++dx)
        for (unsigned dy=0; dy<oh; ++dy) map[(y+dy)*SNIPER_MAP_SIDE+x+dx]=0;
    for (unsigned dx=0; dx<nw; ++dx)
        for (unsigned dy=0; dy<nh; ++dy) map[(y+dy)*SNIPER_MAP_SIDE+x+dx]=250;
    map[y*SNIPER_MAP_SIDE+x]=tile;
    return 0;
}
