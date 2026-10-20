#include "map_painter.h"
#include <string.h>
void sniper_map_painter_destroy(SniperMapPainter *p){
    sniper_surface_destroy(&p->ordinary);
    for(int i=0;i<4;++i)sniper_surface_destroy(&p->special[i]);
}
HRESULT sniper_map_painter_create(SniperMapPainter *p,LPDIRECTDRAW dd,HINSTANCE instance){
    memset(p,0,sizeof *p);
    /* Original402edd..402ef6: resource107,8rows19columns of16px tiles. */
    const SniperSprite ordinary={107,0,0,16,16,8,19,0,0};
    HRESULT hr=sniper_surface_create(dd,instance,&ordinary,&p->ordinary);
    if(FAILED(hr))return hr;
    for(int i=0;i<4;++i){
        hr=sniper_surface_create(dd,instance,&sniper_large_tiles[i],&p->special[i]);
        if(FAILED(hr)){sniper_map_painter_destroy(p);return hr;}
    }
    return DD_OK;
}
static void tile(void *ctx,unsigned id,int32_t x,int32_t y,int32_t frame,int opaque){
    SniperMapPainter *p=ctx;
    SniperSpriteSurface *s=id>=216 && id<=219 ? &p->special[id-216] : &p->ordinary;
    HRESULT hr=sniper_surface_draw(s,p->target,x,y,frame,0,opaque);
    if(FAILED(hr) && SUCCEEDED(p->result))p->result=hr;
}
HRESULT sniper_map_paint(SniperMapPainter *p,LPDIRECTDRAWSURFACE target,const uint8_t *map,
    int32_t cx,int32_t cy,int32_t w,int32_t h,int32_t tick){
    p->target=target;p->result=DD_OK;
    if(sniper_map_draw(map,cx,cy,w,h,tick,tile,p))p->result=E_INVALIDARG;
    p->target=NULL;return p->result;
}
