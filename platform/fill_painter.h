#ifndef SNIPER_FILL_PAINTER_H
#define SNIPER_FILL_PAINTER_H
#include "sprite_surface.h"
#include "../src/surface_fill.h"
/* Adapter target lifetime belongs to caller. Indexed memory path only;
 * backbuffer320 path uses original DirectDraw Blt semantics. */
void sniper_fill_surface_bind(SniperFillSurface *,LPDIRECTDRAWSURFACE);
#endif
