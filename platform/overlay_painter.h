#ifndef SNIPER_OVERLAY_PAINTER_H
#define SNIPER_OVERLAY_PAINTER_H
#include "sprite_surface.h"
#include "../src/pickup_draw.h"
#include "../src/notices.h"
typedef struct {
    SniperSpriteSurface pickup,fonts[4];
    LPDIRECTDRAWSURFACE target;
    HRESULT result;
} SniperOverlayPainter;
HRESULT sniper_overlay_painter_create(SniperOverlayPainter *,LPDIRECTDRAW,HINSTANCE);
void sniper_overlay_painter_destroy(SniperOverlayPainter *);
HRESULT sniper_pickups_paint(SniperOverlayPainter *,LPDIRECTDRAWSURFACE,
    SniperPickupObject *const *,int32_t count,int32_t camera_x,int32_t camera_y);
HRESULT sniper_notices_paint(SniperOverlayPainter *,LPDIRECTDRAWSURFACE,
    const SniperNotices *,int32_t horizontal_extent,int32_t vertical_extent);
#endif
