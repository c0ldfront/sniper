#include "text_panel_painter.h"
#include <string.h>
typedef struct {
    SniperTextPanelPainter *panel;SniperSpriteSurface *fonts;
    DDSURFACEDESC lock;int locked;uint8_t border;HRESULT result;
} Paint;
void sniper_text_panel_close(SniperTextPanelPainter *p) {
    if(p->surface)IDirectDrawSurface_Release(p->surface);
    memset(p,0,sizeof *p);
}
static void line(void *user,int vertical,int32_t x,int32_t y,int32_t length) {
    Paint *c=user;if(!c->locked)return;
    /* Safe clipping for malformed/titleless panel geometry;normal help lines
     * lie within the native allocated extent. */
    for(int32_t i=0;i<length;++i) {
        int32_t px=x+(vertical?0:i),py=y+(vertical?i:0);
        if(px>=0&&py>=0&&px<c->panel->layout.width&&py<c->panel->layout.height)
            ((uint8_t *)c->lock.lpSurface)[py*c->lock.lPitch+px]=c->border;
    }
}
static void unlock(Paint *c) {
    if(c->locked){IDirectDrawSurface_Unlock(c->panel->surface,c->lock.lpSurface);c->locked=0;}
}
static void text(void *user,const char *value,int32_t x,int32_t y,int32_t color) {
    Paint *c=user;unlock(c);if(FAILED(c->result))return;
    if(color<0||color>=4){c->result=E_INVALIDARG;return;}
    for(const unsigned char *p=(const unsigned char *)value;*p;++p) {
        int32_t code=*p<128?*p:(int32_t)*p-256;
        /* Native416090 ignores each4104a0 result. A title wider than its
         * body-derived panel can produce INVALIDRECT for outer glyphs;
         * later in-bounds glyphs must still draw. */
        (void)sniper_surface_draw(c->fonts+color,c->panel->surface,x,y,code-32,0,1);
        x+=6;
    }
}
HRESULT sniper_text_panel_paint(SniperTextPanelPainter *p,LPDIRECTDRAW dd,LPDIRECTDRAWSURFACE target,
    SniperSpriteSurface fonts[4],const char *title,const SniperPanelRow *rows,int32_t count,
    int32_t per,int32_t x,int32_t y,uint8_t background,uint8_t border,int (*recover)(void *),void *user) {
    /* Original412626 releases the cache when IsLost returns DD_OK. Preserve
     * this inverted-looking branch: normally the panel is rebuilt each draw. */
    if(p->surface&&IDirectDrawSurface_IsLost(p->surface)==DD_OK)sniper_text_panel_close(p);
    DDSURFACEDESC target_info={0};target_info.dwSize=sizeof target_info;
    HRESULT hr=IDirectDrawSurface_GetSurfaceDesc(target,&target_info);if(FAILED(hr))return hr;
    if(!p->surface) {
        if(!count)return DD_OK;
        if(!sniper_text_panel_layout(title,rows,count,per,&p->layout))return E_INVALIDARG;
        if(target_info.ddpfPixelFormat.dwRGBBitCount!=8)return DDERR_INVALIDPIXELFORMAT;
        DDSURFACEDESC d={0};d.dwSize=sizeof d;d.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
        d.dwWidth=p->layout.width;d.dwHeight=p->layout.height;d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
        d.ddpfPixelFormat=target_info.ddpfPixelFormat;
        hr=IDirectDraw_CreateSurface(dd,&d,&p->surface,NULL);if(FAILED(hr))return hr;
        LPDIRECTDRAWPALETTE palette=NULL;hr=IDirectDrawSurface_GetPalette(target,&palette);
        if(SUCCEEDED(hr)){hr=IDirectDrawSurface_SetPalette(p->surface,palette);IDirectDrawPalette_Release(palette);}
        if(FAILED(hr)){sniper_text_panel_close(p);return hr;}
        DDBLTFX fill={0};fill.dwSize=sizeof fill;fill.dwFillColor=background;
        do{hr=IDirectDrawSurface_Blt(p->surface,NULL,NULL,NULL,DDBLT_COLORFILL,&fill);}while(hr==DDERR_WASSTILLDRAWING);
        if(FAILED(hr))return hr;
        Paint c={.panel=p,.fonts=fonts,.border=border,.result=DD_OK};c.lock.dwSize=sizeof c.lock;
        c.locked=SUCCEEDED(IDirectDrawSurface_Lock(p->surface,NULL,&c.lock,DDLOCK_WAIT,NULL));
        SniperTextPanelDraw ops={line,text,&c};sniper_text_panel_draw(title,rows,count,per,&p->layout,&ops);unlock(&c);
        if(FAILED(c.result))return c.result;
    }
    if(x==-1)x=((int32_t)target_info.dwWidth-p->layout.width)/2;
    if(y==-1)y=((int32_t)target_info.dwHeight-p->layout.height)/2;
    RECT src={0,0,p->layout.width,p->layout.height},dst={x,y,x+p->layout.width,y+p->layout.height};
    do{hr=IDirectDrawSurface_Blt(target,&dst,p->surface,&src,0,NULL);}while(hr==DDERR_WASSTILLDRAWING);
    if(hr==DDERR_SURFACELOST) {
        IDirectDrawSurface_Restore(p->surface);
        if(recover&&recover(user))return DD_OK; /* original does not redraw here */
    }
    return hr;
}
