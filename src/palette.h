#ifndef SNIPER_PALETTE_H
#define SNIPER_PALETTE_H
#include <stdint.h>
#include <stddef.h>
typedef struct { uint8_t red,green,blue,flags; } SniperPaletteEntry;
/* Resource branch of412150. Returns-1 for malformed/truncated palette input;
 * NULL selects original RGB332 fallback. File-path fallback remains separate. */
int sniper_palette_resource(SniperPaletteEntry out[256],const uint8_t *dib,size_t size);
#endif
