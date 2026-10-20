#include "hud_panel_painter.h"
#include <string.h>
static void remember(SniperHudPanels *p,HRESULT hr){if(FAILED(hr)&&SUCCEEDED(p->result))p->result=hr;}
static void release(void *ctx,SniperPanelSurface s){(void)ctx;IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)s);}
static SniperPanelSurface create(void *ctx,int32_t width,int32_t height)
{
    SniperHudPanels *p=ctx;DDSURFACEDESC d={0};d.dwSize=sizeof d;
    HRESULT hr=IDirectDrawSurface_GetSurfaceDesc(p->target,&d);remember(p,hr);if(FAILED(hr))return 0;
    DDPIXELFORMAT format=d.ddpfPixelFormat;
    if(format.dwRGBBitCount!=8||!(format.dwFlags&DDPF_PALETTEINDEXED8)){remember(p,DDERR_INVALIDPIXELFORMAT);return 0;}
    memset(&d,0,sizeof d);d.dwSize=sizeof d;d.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    d.dwWidth=width;d.dwHeight=height;d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;d.ddpfPixelFormat=format;
    LPDIRECTDRAWSURFACE surface=NULL;hr=IDirectDraw_CreateSurface(p->dd,&d,&surface,NULL);remember(p,hr);if(FAILED(hr))return 0;
    LPDIRECTDRAWPALETTE palette=NULL;hr=IDirectDrawSurface_GetPalette(p->target,&palette);
    if(SUCCEEDED(hr)){hr=IDirectDrawSurface_SetPalette(surface,palette);IDirectDrawPalette_Release(palette);}
    remember(p,hr);if(FAILED(hr)){IDirectDrawSurface_Release(surface);return 0;}
    return (SniperPanelSurface)surface;
}
static void key(void *ctx,SniperPanelSurface s,uint32_t color)
{
    DDCOLORKEY k={color,color};remember(ctx,IDirectDrawSurface_SetColorKey((LPDIRECTDRAWSURFACE)s,DDCKEY_SRCBLT,&k));
}
static void rect(void *ctx,SniperPanelSurface s,int32_t l,int32_t t,int32_t r,int32_t b,uint32_t color)
{
    SniperFillSurface fill;sniper_fill_surface_bind(&fill,(LPDIRECTDRAWSURFACE)s);SniperFillRect bounds={l,t,r,b};
    remember(ctx,(HRESULT)sniper_surface_fill(&fill,0,0,&bounds,color));
}
static void clear(void *ctx,SniperPanelSurface s,uint32_t color)
{
    DDSURFACEDESC d={0};d.dwSize=sizeof d;HRESULT hr=IDirectDrawSurface_GetSurfaceDesc((LPDIRECTDRAWSURFACE)s,&d);
    remember(ctx,hr);if(SUCCEEDED(hr))rect(ctx,s,0,0,(int32_t)d.dwWidth,(int32_t)d.dwHeight,color);
}
static void draw_text(void *ctx,SniperPanelSurface s,const char *text,int32_t x,int32_t y,int color,int opaque)
{
    SniperHudPanels *p=ctx;if(color<0||color>=4){remember(p,E_INVALIDARG);return;}
    for(const unsigned char *c=(const unsigned char *)text;*c;++c){
        int32_t code=*c<128?*c:(int32_t)*c-256;
        remember(p,sniper_surface_draw(&p->fonts[color],(LPDIRECTDRAWSURFACE)s,x,y,code-32,0,opaque));
        x=(int32_t)((uint32_t)x+6u);
    }
}
static void opaque_text(void *c,SniperPanelSurface s,const char *t,int32_t x,int32_t y,int color){draw_text(c,s,t,x,y,color,1);}
static void transparent_text(void *c,SniperPanelSurface s,const char *t,int32_t x,int32_t y,int color){draw_text(c,s,t,x,y,color,0);}
static void blit(void *ctx,SniperPanelSurface s,int32_t x,int32_t y,int32_t width,int32_t height)
{
    SniperHudPanels *p=ctx;LPDIRECTDRAWSURFACE source=(LPDIRECTDRAWSURFACE)s;
    RECT src={0,0,width,height},dst={x,y,(LONG)((uint32_t)x+(uint32_t)width),(LONG)((uint32_t)y+(uint32_t)height)};
    HRESULT hr;do{hr=IDirectDrawSurface_Blt(p->target,&dst,source,&src,DDBLT_KEYSRC,NULL);}while(hr==DDERR_WASSTILLDRAWING);
    if(hr==DDERR_SURFACELOST){IDirectDrawSurface_Restore(source);IDirectDrawSurface_Restore(p->target);}
    remember(p,hr);
}
void sniper_hud_panels_init(SniperHudPanels *p,LPDIRECTDRAW dd,SniperSpriteSurface fonts[4])
{
    memset(p,0,sizeof *p);p->dd=dd;p->fonts=fonts;
    p->team.ops=(SniperPanelOps){release,create,key,clear,opaque_text,blit,p};
    p->equipment.cache.ops=p->team.ops;p->equipment.rect=rect;p->equipment.transparent_text=transparent_text;
}
void sniper_hud_panels_destroy(SniperHudPanels *p)
{
    if(p->team.surface)release(p,p->team.surface);
    if(p->equipment.cache.surface)release(p,p->equipment.cache.surface);
    p->team.surface=p->equipment.cache.surface=0;
}
HRESULT sniper_hud_panels_paint(SniperHudPanels *p,LPDIRECTDRAWSURFACE target,SniperShipObject *local,int32_t width,int32_t height,uint32_t background,uint32_t highlight)
{
    if(!p||!p->dd||!p->fonts||!target||!local)return E_INVALIDARG;
    p->target=target;p->result=DD_OK;p->team.local=local;p->team.screen_width=width;p->team.background=background;
    p->equipment.cache.local=local;p->equipment.cache.background=background;p->equipment.screen_height=height;p->equipment.highlight=highlight;
    int failed=sniper_team_panel_draw(&p->team);
    if(!failed)failed=sniper_equipment_panel_draw(&p->equipment);
    p->target=NULL;return FAILED(p->result)?p->result:failed?E_FAIL:DD_OK;
}
HRESULT sniper_hud_paint(SniperHudPanels *p,LPDIRECTDRAWSURFACE target,SniperHudStatus *h,uint32_t background,uint32_t highlight)
{
    if(!p||!h||!h->local||!target)return E_INVALIDARG;
    HRESULT hr=sniper_hud_status_paint(h,p->fonts,target);
    HRESULT panels=sniper_hud_panels_paint(p,target,h->local,h->width,h->height,background,highlight);
    return FAILED(hr)?hr:panels;
}
