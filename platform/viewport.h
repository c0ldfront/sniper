#ifndef SNIPER_VIEWPORT_H
#define SNIPER_VIEWPORT_H
#define COBJMACROS
#include <windows.h>
#include <ddraw.h>
/* Original402c7f..402cd9: attach a rectangular clip list to backbuffer.
 * Surface retains clipper reference. Reapply after viewport size changes. */
HRESULT sniper_viewport_clip(LPDIRECTDRAW,LPDIRECTDRAWSURFACE,int width,int height);
#endif
