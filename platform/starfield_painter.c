#include "starfield_painter.h"
static HRESULT indexed(LPDIRECTDRAWSURFACE surface)
{
    DDPIXELFORMAT format={0};format.dwSize=sizeof format;
    HRESULT hr=IDirectDrawSurface_GetPixelFormat(surface,&format);
    if(FAILED(hr))return hr;
    return format.dwRGBBitCount==8 && (format.dwFlags&DDPF_PALETTEINDEXED8)?DD_OK:DDERR_INVALIDPIXELFORMAT;
}
HRESULT sniper_indexed_color(LPDIRECTDRAWSURFACE surface,COLORREF color,uint8_t *out)
{
    if(!surface||!out)return E_INVALIDARG;
    HRESULT hr=indexed(surface);if(FAILED(hr))return hr;
    HDC dc;hr=IDirectDrawSurface_GetDC(surface,&dc);
    if(FAILED(hr))return hr;
    COLORREF saved=GetPixel(dc,0,0);
    if(saved==CLR_INVALID){IDirectDrawSurface_ReleaseDC(surface,dc);return E_FAIL;}
    COLORREF written=SetPixel(dc,0,0,color);
    HRESULT release=IDirectDrawSurface_ReleaseDC(surface,dc);
    if(written==CLR_INVALID)return E_FAIL;
    DDSURFACEDESC d={0};d.dwSize=sizeof d;
    hr=release;
    if(SUCCEEDED(hr)){
        do{hr=IDirectDrawSurface_Lock(surface,NULL,&d,0,NULL);}while(hr==DDERR_WASSTILLDRAWING);
        if(SUCCEEDED(hr)){
            *out=*(const uint8_t *)d.lpSurface;
            hr=IDirectDrawSurface_Unlock(surface,NULL);
        }
    }
    HRESULT restore=IDirectDrawSurface_GetDC(surface,&dc);
    if(SUCCEEDED(restore)){
        if(SetPixel(dc,0,0,saved)==CLR_INVALID)restore=E_FAIL;
        release=IDirectDrawSurface_ReleaseDC(surface,dc);
        if(SUCCEEDED(restore))restore=release;
    }
    return FAILED(hr)?hr:restore;
}
HRESULT sniper_starfield_colors(LPDIRECTDRAWSURFACE surface,uint8_t *near_color,uint8_t *far_color)
{
    if(!surface||!near_color||!far_color)return E_INVALIDARG;
    HRESULT hr=indexed(surface);if(FAILED(hr))return hr;
    hr=sniper_indexed_color(surface,RGB(92,92,92),near_color);
    if(FAILED(hr))return hr;
    return sniper_indexed_color(surface,RGB(48,48,48),far_color);
}
HRESULT sniper_starfield_paint(const SniperStarfield *field,LPDIRECTDRAWSURFACE target,
    int32_t width,int32_t height,int32_t camera_x,int32_t camera_y,uint8_t near_color,uint8_t far_color)
{
    if(!field||!target)return E_INVALIDARG;
    HRESULT hr=indexed(target);if(FAILED(hr))return hr;
    DDSURFACEDESC d={0};d.dwSize=sizeof d;
    hr=IDirectDrawSurface_Lock(target,NULL,&d,DDLOCK_WAIT,NULL);
    if(FAILED(hr))return hr;
    int result=-1;
    if(width>0&&height>0&&(DWORD)width<=d.dwWidth&&(DWORD)height<=d.dwHeight)
        result=sniper_starfield_draw(field,d.lpSurface,d.lPitch,width,height,camera_x,camera_y,near_color,far_color);
    hr=IDirectDrawSurface_Unlock(target,d.lpSurface);
    return result?E_INVALIDARG:hr;
}
