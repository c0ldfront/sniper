#include "sprite_surface.h"
#include <string.h>
#include <io.h>
/* Explicit filename constructors402db0..40337a. Descriptor-only constructor
 * 4101e0 (large tiles) supplies no filename. */
const char *sniper_surface_override_name(int32_t resource) {
    switch(resource) {
    case 139:return "font1.bmp"; case 107:return "tiles.bmp";
    case 109:return "bullets.bmp"; case 145:return "bombs.bmp";
    case 142:return "prizes.bmp"; case 147:return "exhaust.bmp";
    case 158:return "turret.bmp"; case 172:return "turret2.bmp";
    case 138:return "explode0.bmp"; case 141:return "explode1.bmp";
    case 146:return "explode2.bmp"; case 176:return "repel.bmp";
    case 178:return "warp.bmp"; case 173:return "damage.bmp";
    case 700:return "ships.bmp"; case 159:return "edit.bmp";
    default:return NULL;
    }
}
HBITMAP sniper_surface_load_bitmap(HINSTANCE instance,int32_t resource) {
    const char *name=sniper_surface_override_name(resource);
    /* Recheck existence on every restore. An existing invalid bitmap must not
     * silently fall back to the resource (4103ff..410438). */
    if(name && _access(name,0)==0)
        return LoadImageA(NULL,name,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_LOADFROMFILE);
    return LoadImageA(instance,MAKEINTRESOURCEA(resource),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
}
/* Original412300 resolves COLORREF black to surface-native pixel value by
 * temporarily writing pixel0,0, locking the surface, then restoring that pixel. */
static HRESULT black_key(LPDIRECTDRAWSURFACE surface) {
    HDC dc;COLORREF saved;HRESULT hr=IDirectDrawSurface_GetDC(surface,&dc);
    if(FAILED(hr))return hr;
    saved=GetPixel(dc,0,0);SetPixel(dc,0,0,RGB(0,0,0));
    IDirectDrawSurface_ReleaseDC(surface,dc);
    DDSURFACEDESC desc;memset(&desc,0,sizeof desc);desc.dwSize=sizeof desc;
    do {hr=IDirectDrawSurface_Lock(surface,NULL,&desc,0,NULL);}while(hr==DDERR_WASSTILLDRAWING);
    DWORD pixel=0;
    if(SUCCEEDED(hr)) {
        DWORD bits=desc.ddpfPixelFormat.dwRGBBitCount;
        /* Read only bytes present for this pixel, avoiding original DWORD overread. */
        memcpy(&pixel,desc.lpSurface,(bits+7)/8>4 ? 4 : (bits+7)/8);
        if(bits<32)pixel&=(1u<<bits)-1;
        IDirectDrawSurface_Unlock(surface,NULL);
    }
    HRESULT dc_hr=IDirectDrawSurface_GetDC(surface,&dc);
    if(SUCCEEDED(dc_hr)){SetPixel(dc,0,0,saved);IDirectDrawSurface_ReleaseDC(surface,dc);}
    if(FAILED(hr))return hr;
    if(FAILED(dc_hr))return dc_hr;
    DDCOLORKEY key={pixel,pixel};
    return IDirectDrawSurface_SetColorKey(surface,DDCKEY_SRCBLT,&key);
}
HRESULT sniper_surface_restore(SniperSpriteSurface *s) {
    HRESULT hr=IDirectDrawSurface_Restore(s->surface);if(FAILED(hr))return hr;
    HBITMAP bitmap=sniper_surface_load_bitmap(s->instance,s->descriptor.resource);
    if(!bitmap)return E_FAIL;
    HDC source=CreateCompatibleDC(NULL);
    if(!source){DeleteObject(bitmap);return E_FAIL;}
    HGDIOBJ old=SelectObject(source,bitmap);HDC target;
    hr=IDirectDrawSurface_GetDC(s->surface,&target);
    if(SUCCEEDED(hr)) {
        int w=s->descriptor.columns*s->descriptor.width;
        int h=s->descriptor.rows*s->descriptor.height;
        BOOL copied=StretchBlt(target,0,0,w,h,source,s->descriptor.origin_x,s->descriptor.origin_y,w,h,SRCCOPY);
        IDirectDrawSurface_ReleaseDC(s->surface,target);
        if(!copied)hr=E_FAIL;
    }
    SelectObject(source,old);DeleteDC(source);DeleteObject(bitmap);
    return FAILED(hr) ? hr : black_key(s->surface);
}
HRESULT sniper_surface_create(LPDIRECTDRAW dd,HINSTANCE instance,const SniperSprite *desc,SniperSpriteSurface *out) {
    memset(out,0,sizeof *out);out->descriptor=*desc;out->instance=instance;
    DDSURFACEDESC d;memset(&d,0,sizeof d);d.dwSize=sizeof d;
    d.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY;
    d.dwWidth=desc->width*desc->columns;d.dwHeight=desc->height*desc->rows;
    HRESULT hr=IDirectDraw_CreateSurface(dd,&d,&out->surface,NULL);
    if(SUCCEEDED(hr))hr=sniper_surface_restore(out);
    if(FAILED(hr))sniper_surface_destroy(out);
    return hr;
}
HRESULT sniper_surface_draw(SniperSpriteSurface *s,LPDIRECTDRAWSURFACE target,
    int32_t x,int32_t y,int32_t frame,int centered,int opaque) {
    SniperBlit b;
    if(sniper_sprite_blit(&s->descriptor,x,y,frame,centered,opaque,&b))return E_INVALIDARG;
    RECT src={b.source.left,b.source.top,b.source.right,b.source.bottom};
    RECT dst={b.destination.left,b.destination.top,b.destination.right,b.destination.bottom};
    HRESULT hr;
    do {hr=IDirectDrawSurface_Blt(target,&dst,s->surface,&src,b.flags,NULL);}while(hr==DDERR_WASSTILLDRAWING);
    /*41059d..4105b1 returns target Restore result, not the failed Blt. */
    if(hr==DDERR_SURFACELOST){sniper_surface_restore(s);hr=IDirectDrawSurface_Restore(target);}
    return hr;
}
void sniper_surface_destroy(SniperSpriteSurface *s){
    if(s->surface)IDirectDrawSurface_Release(s->surface);
    s->surface=NULL;
}
