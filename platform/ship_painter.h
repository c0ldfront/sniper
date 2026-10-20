#ifndef SNIPER_SHIP_PAINTER_H
#define SNIPER_SHIP_PAINTER_H
#include "sprite_surface.h"
#include "../src/ship_render.h"
#include "../src/ship_draw_order.h"
typedef struct {
    SniperSpriteSurface body[9],effects[2],fonts[4];
    LPDIRECTDRAWSURFACE target;
    HRESULT result;
} SniperShipPainter;
HRESULT sniper_ship_painter_create(SniperShipPainter *,LPDIRECTDRAW,HINSTANCE);
void sniper_ship_painter_destroy(SniperShipPainter *);
HRESULT sniper_ship_paint(SniperShipPainter *,LPDIRECTDRAWSURFACE,
    SniperShipObject *,const SniperShipObject *,SniperShipObject *,uint32_t *,
    int low_detail,int label_mode,int32_t upper,int32_t lower,int32_t bonus,
    int32_t camera_x,int32_t camera_y);
typedef struct {
    SniperShipObject **players; /* ID-indexed1024 entries for parent lookup */
    uint32_t *blink;
    int low_detail,label_mode;
    int32_t upper,lower,bonus;
} SniperShipPaintOptions;
HRESULT sniper_ships_paint(SniperShipPainter *,LPDIRECTDRAWSURFACE,
    SniperShipDrawOrder *,const SniperShipPaintOptions *);
#endif
