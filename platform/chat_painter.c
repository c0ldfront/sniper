#include "chat_painter.h"
#include <string.h>
typedef struct {LPDIRECTDRAWSURFACE target;SniperSpriteSurface *fonts;int opaque;HRESULT result;} Text;
static void text(void *user,const char *value,int32_t x,int32_t y,int32_t color) {
    Text *c=user;if(FAILED(c->result))return;
    if(color<0||color>=4){c->result=E_INVALIDARG;return;}
    for(const unsigned char *p=(const unsigned char *)value;*p;++p) {
        int32_t code=*p<128?*p:(int32_t)*p-256;
        /* Original text helpers continue after individual glyph failures. */
        (void)sniper_surface_draw(c->fonts+color,c->target,x,y,code-32,0,c->opaque);
        x+=6;
    }
}
void sniper_chat_close(SniperChatPainter *p) {
    if(p->surface)IDirectDrawSurface_Release(p->surface);
    memset(p,0,sizeof *p);
}
static uint32_t milliseconds(SniperChatPaint *s) {return s->milliseconds?s->milliseconds(s->user):(uint32_t)GetTickCount();}
HRESULT sniper_chat_paint(SniperChatPainter *p,SniperChatPaint *s) {
    if(!p||!s||!s->dirty)return E_INVALIDARG;
    if((s->enabled&&!*s->enabled)||s->rows<3)return DD_OK;
    if(!s->draw||!s->target||!s->fonts||!s->lines||!s->colors)return E_INVALIDARG;
    if(sniper_chat_ticker_due(milliseconds(s),p->last_build_tick,s->ticker_interval,s->ticker_enabled)&&s->ticker)s->ticker(s->user);
    HRESULT hr=DD_OK;
    if(*s->dirty||!p->surface) {
        uint32_t tick=milliseconds(s)/10;
        sniper_chat_close(p);p->last_build_tick=tick;*s->dirty=0;
        SniperChatLayout layout;
        if(!sniper_chat_layout(s->width,s->rows,&layout))return E_INVALIDARG;
        DDSURFACEDESC target={0};target.dwSize=sizeof target;
        hr=IDirectDrawSurface_GetSurfaceDesc(s->target,&target);if(FAILED(hr))return hr;
        if(target.ddpfPixelFormat.dwRGBBitCount!=8)return DDERR_INVALIDPIXELFORMAT;
        DDSURFACEDESC desc={0};desc.dwSize=sizeof desc;
        desc.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
        desc.dwWidth=layout.width;desc.dwHeight=layout.height;
        desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;desc.ddpfPixelFormat=target.ddpfPixelFormat;
        hr=IDirectDraw_CreateSurface(s->draw,&desc,&p->surface,NULL);if(FAILED(hr))return hr;
        LPDIRECTDRAWPALETTE palette=NULL;hr=IDirectDrawSurface_GetPalette(s->target,&palette);
        if(SUCCEEDED(hr)){hr=IDirectDrawSurface_SetPalette(p->surface,palette);IDirectDrawPalette_Release(palette);}
        if(FAILED(hr))goto rebuild_failed;
        DDBLTFX fill={0};fill.dwSize=sizeof fill;fill.dwFillColor=s->background;
        do{hr=IDirectDrawSurface_Blt(p->surface,NULL,NULL,NULL,DDBLT_COLORFILL,&fill);}while(hr==DDERR_WASSTILLDRAWING);
        if(FAILED(hr))goto rebuild_failed;
        Text c={p->surface,s->fonts,1,DD_OK};
        sniper_chat_history_draw(&layout,s->lines,s->colors,s->rows,text,&c);
        hr=c.result;if(FAILED(hr))goto rebuild_failed;
        DDCOLORKEY key={s->background,s->background};
        hr=IDirectDrawSurface_SetColorKey(p->surface,DDCKEY_SRCBLT,&key);if(FAILED(hr))goto rebuild_failed;
    }
    DDSURFACEDESC desc={0};desc.dwSize=sizeof desc;
    hr=IDirectDrawSurface_GetSurfaceDesc(p->surface,&desc);if(FAILED(hr))return hr;
    SniperChatRect placement;Text c={s->target,s->fonts,0,DD_OK};
    sniper_chat_placement(s->height,(int32_t)desc.dwWidth,(int32_t)desc.dwHeight,s->input,&placement,text,&c);
    if(FAILED(c.result))return c.result;
    RECT src={0,0,(LONG)desc.dwWidth,(LONG)desc.dwHeight};
    RECT dst={placement.left,placement.top,placement.right,placement.bottom};
    do{hr=IDirectDrawSurface_Blt(s->target,&dst,p->surface,&src,DDBLT_KEYSRC,NULL);}while(hr==DDERR_WASSTILLDRAWING);
    if(hr==DDERR_SURFACELOST&&s->recover&&s->recover(s->user))return DD_OK;
    return hr;
rebuild_failed:
    IDirectDrawSurface_Release(p->surface);p->surface=NULL;*s->dirty=1;return hr;
}
