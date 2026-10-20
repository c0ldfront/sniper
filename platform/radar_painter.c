#include "radar_painter.h"
#include <stdlib.h>
#include <string.h>
void sniper_radar_close(SniperRadarPainter *p){if(p->surface)IDirectDrawSurface_Release(p->surface);memset(p,0,sizeof *p);}
typedef struct {SniperFillSurface fill;int32_t width;HRESULT result;} Mark;
static void marker(void *user,int32_t l,int32_t t,int32_t r,int32_t b,uint32_t color) {
    Mark *m=user;SniperFillRect rect={l,t,r,b};
    if(SUCCEEDED(m->result))m->result=(HRESULT)sniper_surface_fill(&m->fill,1,m->width,&rect,color);
}
HRESULT sniper_radar_paint(SniperRadarPainter *p,SniperRadarPaint *s) {
    HRESULT hr;
    if(*s->dirty||!p->surface||p->scale!=s->scale||p->small!=s->small) {
        sniper_radar_close(p);int32_t side;
        if(!sniper_radar_map_size(s->width,s->scale,s->small,&side)||side<=0||side>4096)return E_INVALIDARG;
        DDSURFACEDESC target={0};target.dwSize=sizeof target;
        hr=IDirectDrawSurface_GetSurfaceDesc(s->target,&target);if(FAILED(hr))return hr;
        if(target.ddpfPixelFormat.dwRGBBitCount!=8)return DDERR_INVALIDPIXELFORMAT;
        DDSURFACEDESC d={0};d.dwSize=sizeof d;d.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
        d.dwWidth=d.dwHeight=side;d.ddsCaps.dwCaps=DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN;d.ddpfPixelFormat=target.ddpfPixelFormat;
        hr=IDirectDraw_CreateSurface(s->draw,&d,&p->surface,NULL);
        if(FAILED(hr)){*s->enabled=0;return DD_OK;}
        p->size=side;p->scale=s->scale;p->small=s->small;
        LPDIRECTDRAWPALETTE palette=NULL;hr=IDirectDrawSurface_GetPalette(s->target,&palette);
        if(SUCCEEDED(hr)){hr=IDirectDrawSurface_SetPalette(p->surface,palette);IDirectDrawPalette_Release(palette);}
        if(FAILED(hr))return hr;
        /* Always initialize new storage, including scale changes. Original's
         * dirty0 allocation path could leave the new bitmap uninitialized. */
        *s->dirty=1;
    }
    if(*s->dirty) {
        DDSURFACEDESC lock={0};lock.dwSize=sizeof lock;
        hr=IDirectDrawSurface_Lock(p->surface,NULL,&lock,DDLOCK_WAIT,NULL);if(FAILED(hr))return hr;
        if(lock.lPitch<p->size){IDirectDrawSurface_Unlock(p->surface,lock.lpSurface);return E_FAIL;}
        size_t bytes=(size_t)(p->size+1)*(size_t)lock.lPitch+(size_t)p->size+1;
        uint8_t *scratch=malloc(bytes);
        if(!scratch){IDirectDrawSurface_Unlock(p->surface,lock.lpSurface);return E_OUTOFMEMORY;}
        memset(scratch,s->colors[0],bytes);
        int ok=sniper_radar_map_pixels(scratch,bytes,lock.lPitch,p->size,p->size,s->map,s->colors[1],s->colors[2]);
        if(ok)for(int y=0;y<p->size;++y)memcpy((uint8_t *)lock.lpSurface+y*lock.lPitch,scratch+y*lock.lPitch,(size_t)p->size);
        free(scratch);IDirectDrawSurface_Unlock(p->surface,lock.lpSurface);if(!ok)return E_FAIL;
        *s->dirty=0;
    }
    SniperRadarView view;sniper_radar_view(s->width,s->height,s->camera_x,s->camera_y,p->size,p->size,s->small,&view);
    Mark m={.width=s->width,.result=DD_OK};sniper_fill_surface_bind(&m.fill,s->target);
    int32_t x=view.destination_x,y=view.destination_y,n=view.size;
    if(n<=0||view.source_x<0||view.source_y<0)return E_INVALIDARG;
    hr=(HRESULT)sniper_surface_horizontal(&m.fill,1,s->width,x-1,y-1,n+2,s->colors[15]);if(FAILED(hr))return hr;
    hr=(HRESULT)sniper_surface_vertical(&m.fill,1,s->width,x-1,y-1,n+2,s->colors[15]);if(FAILED(hr))return hr;
    hr=(HRESULT)sniper_surface_horizontal(&m.fill,1,s->width,x-1,y+n,n+2,s->colors[15]);if(FAILED(hr))return hr;
    hr=(HRESULT)sniper_surface_vertical(&m.fill,1,s->width,x+n,y-1,n+2,s->colors[15]);if(FAILED(hr))return hr;
    RECT src={view.source_x,view.source_y,view.source_x+n,view.source_y+n},dst={x,y,x+n,y+n};
    do{hr=IDirectDrawSurface_Blt(s->target,&dst,p->surface,&src,0,NULL);}while(hr==DDERR_WASSTILLDRAWING);
    if(hr==DDERR_SURFACELOST&&s->recover&&s->recover(s->user))return DD_OK;
    if(FAILED(hr))return hr;
    SniperRadarMarkers markers=s->markers;markers.view=view;markers.screen_width=s->width;markers.map_width=markers.map_height=p->size;
    markers.friendly=s->colors[11];markers.enemy_positive=s->colors[6];markers.enemy_nonpositive=s->colors[7];
    markers.projectile_color=s->colors[8];markers.pickup_color=s->colors[10];markers.rect=marker;markers.user=&m;
    sniper_radar_markers(&markers);return m.result;
}
