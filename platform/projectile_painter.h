#ifndef SNIPER_PROJECTILE_PAINTER_H
#define SNIPER_PROJECTILE_PAINTER_H
#include "sprite_surface.h"
#include "../src/projectile_draw.h"
typedef struct { SniperSpriteSurface assets[5];LPDIRECTDRAWSURFACE target;HRESULT result; } SniperProjectilePainter;
HRESULT sniper_projectile_painter_create(SniperProjectilePainter *,LPDIRECTDRAW,HINSTANCE);
void sniper_projectile_painter_destroy(SniperProjectilePainter *);
HRESULT sniper_projectiles_paint(SniperProjectilePainter *,LPDIRECTDRAWSURFACE,
    SniperProjectile *const *,int32_t count,int32_t camera_x,int32_t camera_y,int suppress_impacts);
#endif
