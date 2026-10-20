#ifndef SNIPER87_MAP_H
#define SNIPER87_MAP_H
#include <stdint.h>
#define SNIPER_MAP_SIDE 1024
#define SNIPER_MAP_BYTES (SNIPER_MAP_SIDE * SNIPER_MAP_SIDE)
typedef struct { int32_t width, height; } SniperTileSize;
/* Recovered 0x404240: clears old large-tile footprint, reserves new footprint
 * with 250, then writes its origin. Returns -1 for undefined input (outside
 * map or absent descriptor), before changing memory. */
int sniper_map_place(uint8_t map[SNIPER_MAP_BYTES],
                     const SniperTileSize sizes[256],
                     unsigned x, unsigned y, uint8_t tile);
#endif
