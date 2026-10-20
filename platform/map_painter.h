#ifndef SNIPER_MAP_PAINTER_H
#define SNIPER_MAP_PAINTER_H
#include "sprite_surface.h"
#include "../src/map_render.h"
typedef struct {
    SniperSpriteSurface ordinary,special[4];
    LPDIRECTDRAWSURFACE target;HRESULT result;
} SniperMapPainter;
HRESULT sniper_map_painter_create(SniperMapPainter *,LPDIRECTDRAW,HINSTANCE);
void sniper_map_painter_destroy(SniperMapPainter *);
/* Target must have viewport clipping configured for edge-crossing blits. */
HRESULT sniper_map_paint(SniperMapPainter *,LPDIRECTDRAWSURFACE,const uint8_t *,
    int32_t camera_x,int32_t camera_y,int32_t width,int32_t height,int32_t tick);
#endif
