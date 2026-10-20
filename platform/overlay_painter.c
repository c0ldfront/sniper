#include "overlay_painter.h"
#include "../src/font.h"
#include <string.h>
void sniper_overlay_painter_destroy(SniperOverlayPainter *p) {
    sniper_surface_destroy(&p->pickup);
    for(int i=0;i<4;++i)sniper_surface_destroy(&p->fonts[i]);
}
HRESULT sniper_overlay_painter_create(SniperOverlayPainter *p,LPDIRECTDRAW dd,HINSTANCE instance) {
    memset(p,0,sizeof *p);
    HRESULT hr=sniper_surface_create(dd,instance,&sniper_pickup_sprite,&p->pickup);
    if(FAILED(hr))return hr;
    for(int i=0;i<4;++i){hr=sniper_surface_create(dd,instance,&sniper_fonts[i],&p->fonts[i]);if(FAILED(hr)){sniper_overlay_painter_destroy(p);return hr;}}
    return DD_OK;
}
static void pickup(void *context,int32_t x,int32_t y,int32_t frame,int center,int opaque) {
    SniperOverlayPainter *p=context;
    HRESULT hr=sniper_surface_draw(&p->pickup,p->target,x,y,frame,center,opaque);
    if(FAILED(hr) && SUCCEEDED(p->result))p->result=hr;
}
static void glyph(void *context,int color,int32_t x,int32_t y,int32_t frame) {
    SniperOverlayPainter *p=context;
    if(color<0 || color>=4){p->result=E_INVALIDARG;return;}
    HRESULT hr=sniper_surface_draw(&p->fonts[color],p->target,x,y,frame,0,0);
    if(FAILED(hr) && SUCCEEDED(p->result))p->result=hr;
}
static void notice(void *context,const char *text,int32_t x,int32_t y,int32_t color) {
    sniper_font_draw(text,x,y,color,glyph,context);
}
HRESULT sniper_pickups_paint(SniperOverlayPainter *p,LPDIRECTDRAWSURFACE target,
    SniperPickupObject *const *items,int32_t count,int32_t cx,int32_t cy) {
    p->target=target;p->result=DD_OK;
    for(int32_t i=0;i<count;++i)sniper_pickup_draw(items[i],cx,cy,pickup,p);
    p->target=NULL;return p->result;
}
HRESULT sniper_notices_paint(SniperOverlayPainter *p,LPDIRECTDRAWSURFACE target,
    const SniperNotices *n,int32_t width,int32_t height) {
    p->target=target;p->result=DD_OK;sniper_notices_draw(n,width,height,notice,p);
    p->target=NULL;return p->result;
}
