#include "fill_painter.h"
static uint32_t blt(void *ctx,const SniperFillRect *r,uint32_t color)
{
    RECT rect={r->left,r->top,r->right,r->bottom};
    DDBLTFX fx={0};fx.dwSize=sizeof fx;fx.dwFillColor=color;
    return (uint32_t)IDirectDrawSurface_Blt((LPDIRECTDRAWSURFACE)ctx,&rect,NULL,NULL,DDBLT_COLORFILL,&fx);
}
static uint32_t lock(void *ctx,SniperFillBuffer *b)
{
    LPDIRECTDRAWSURFACE target=ctx;DDPIXELFORMAT f={0};f.dwSize=sizeof f;
    HRESULT hr=IDirectDrawSurface_GetPixelFormat(target,&f);if(FAILED(hr))return (uint32_t)hr;
    if(f.dwRGBBitCount!=8||!(f.dwFlags&DDPF_PALETTEINDEXED8))return (uint32_t)DDERR_INVALIDPIXELFORMAT;
    DDSURFACEDESC d={0};d.dwSize=sizeof d;
    hr=IDirectDrawSurface_Lock(target,NULL,&d,DDLOCK_WAIT,NULL);
    if(SUCCEEDED(hr)){b->pixels=d.lpSurface;b->pitch=d.lPitch;b->width=(int32_t)d.dwWidth;b->height=(int32_t)d.dwHeight;}
    return (uint32_t)hr;
}
static uint32_t unlock(void *ctx,uint8_t *pixels){return (uint32_t)IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)ctx,pixels);}
static uint32_t restore(void *ctx){return (uint32_t)IDirectDrawSurface_Restore((LPDIRECTDRAWSURFACE)ctx);}
void sniper_fill_surface_bind(SniperFillSurface *s,LPDIRECTDRAWSURFACE target)
{
    *s=(SniperFillSurface){blt,lock,unlock,restore,target};
}
