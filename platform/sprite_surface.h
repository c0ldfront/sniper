#ifndef SNIPER_PLATFORM_SPRITE_SURFACE_H
#define SNIPER_PLATFORM_SPRITE_SURFACE_H
#define COBJMACROS
#include <windows.h>
#include <ddraw.h>
#include "../src/sprite.h"
typedef struct {
    LPDIRECTDRAWSURFACE surface;
    SniperSprite descriptor;
    HINSTANCE instance;
} SniperSpriteSurface;
/* Original relative override filenames; NULL for resource-only sprites. */
const char *sniper_surface_override_name(int32_t resource);
HBITMAP sniper_surface_load_bitmap(HINSTANCE,int32_t resource);
HRESULT sniper_surface_create(LPDIRECTDRAW,HINSTANCE,const SniperSprite *,SniperSpriteSurface *);
HRESULT sniper_surface_restore(SniperSpriteSurface *);
HRESULT sniper_surface_draw(SniperSpriteSurface *,LPDIRECTDRAWSURFACE target,
    int32_t x,int32_t y,int32_t frame,int centered,int opaque);
void sniper_surface_destroy(SniperSpriteSurface *);
#endif
