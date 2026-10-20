#include "game_display.h"
#include "viewport.h"
#include <stdint.h>
#include <string.h>
static int32_t bits(uint32_t value){int32_t result;memcpy(&result,&value,4);return result;}
void sniper_game_display_close(SniperGameDisplay *d) {
    if(d->backbuffer)IDirectDrawSurface_Release(d->backbuffer);
    if(d->primary)IDirectDrawSurface_Release(d->primary);
    if(d->draw) {
        IDirectDraw_RestoreDisplayMode(d->draw);
        IDirectDraw_SetCooperativeLevel(d->draw,d->window,DDSCL_NORMAL);
        IDirectDraw_Release(d->draw);
    }
    memset(d,0,sizeof *d);
}
HRESULT sniper_game_display_open(SniperGameDisplay *d,HWND window,int width,int height) {
    if(d->draw || width<=0 || height<=0)return E_INVALIDARG;
    d->window=window;d->width=width;d->height=height;
    HRESULT hr=DirectDrawCreate(NULL,&d->draw,NULL);if(FAILED(hr))goto fail;
    hr=IDirectDraw_SetCooperativeLevel(d->draw,window,0x51);if(FAILED(hr))goto fail;
    hr=IDirectDraw_SetDisplayMode(d->draw,width,height,8);if(FAILED(hr))goto fail;
    /* Preserve original316-byte DDCAPS request. Zero initialization is a safe
     * fallback if GetCaps fails;original read an uninitialized total then. */
    DWORD hardware[79]={0},emulation[79]={0};hardware[0]=emulation[0]=316;
    IDirectDraw_GetCaps(d->draw,(LPDDCAPS)hardware,(LPDDCAPS)emulation);
    uint32_t required=(uint32_t)width*(uint32_t)height*3u+0x80000u;
    d->backbuffer_count=bits(required)<bits(hardware[15])?2:1;
    DDSURFACEDESC desc={0};desc.dwSize=sizeof desc;desc.dwFlags=0x21;
    desc.dwBackBufferCount=(DWORD)d->backbuffer_count;desc.ddsCaps.dwCaps=0x218;
    hr=IDirectDraw_CreateSurface(d->draw,&desc,&d->primary,NULL);if(FAILED(hr))goto fail;
    DDSCAPS caps={0};caps.dwCaps=DDSCAPS_BACKBUFFER;
    hr=IDirectDrawSurface_GetAttachedSurface(d->primary,&caps,&d->backbuffer);if(FAILED(hr))goto fail;
    hr=sniper_viewport_clip(d->draw,d->backbuffer,width,height);if(FAILED(hr))goto fail;
    return DD_OK;
fail:
    sniper_game_display_close(d);return hr;
}
