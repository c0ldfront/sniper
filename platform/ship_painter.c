#include "ship_painter.h"
#include "../src/ship_assets.h"
#include "../src/font.h"
#include <string.h>
void sniper_ship_painter_destroy(SniperShipPainter *p){
    for(int i=0;i<9;++i)sniper_surface_destroy(&p->body[i]);
    for(int i=0;i<2;++i)sniper_surface_destroy(&p->effects[i]);
    for(int i=0;i<4;++i)sniper_surface_destroy(&p->fonts[i]);
}
HRESULT sniper_ship_painter_create(SniperShipPainter *p,LPDIRECTDRAW dd,HINSTANCE instance){
    memset(p,0,sizeof *p);HRESULT hr;
    for(int i=0;i<9;++i){hr=sniper_surface_create(dd,instance,&sniper_ship_assets[i],&p->body[i]);if(FAILED(hr))goto fail;}
    for(int i=0;i<2;++i){hr=sniper_surface_create(dd,instance,&sniper_effect_assets[i],&p->effects[i]);if(FAILED(hr))goto fail;}
    for(int i=0;i<4;++i){hr=sniper_surface_create(dd,instance,&sniper_fonts[i],&p->fonts[i]);if(FAILED(hr))goto fail;}
    return DD_OK;
fail:sniper_ship_painter_destroy(p);return hr;
}
static void draw(SniperShipPainter *p,SniperSpriteSurface *s,int32_t x,int32_t y,int32_t frame,int center){
    HRESULT hr=sniper_surface_draw(s,p->target,x,y,frame,center,0);
    if(FAILED(hr) && SUCCEEDED(p->result))p->result=hr;
}
static void body(void *ctx,int slot,int32_t x,int32_t y,int32_t frame){
    SniperShipPainter *p=ctx;
    if(slot<0 || slot>=9){p->result=E_INVALIDARG;return;}
    draw(p,&p->body[slot],x,y,frame,1);
}
static void effect(void *ctx,int resource,int32_t x,int32_t y,int32_t frame){
    SniperShipPainter *p=ctx;
    if(resource!=147 && resource!=173){p->result=E_INVALIDARG;return;}
    draw(p,&p->effects[resource==173],x,y,frame,1);
}
static void glyph(void *ctx,int color,int32_t x,int32_t y,int32_t frame){
    SniperShipPainter *p=ctx;
    if(color<0 || color>=4){p->result=E_INVALIDARG;return;}
    draw(p,&p->fonts[color],x,y,frame,0);
}
static void text(void *ctx,const char *str,int32_t x,int32_t y,int color){sniper_font_draw(str,x,y,color,glyph,ctx);}
HRESULT sniper_ship_paint(SniperShipPainter *p,LPDIRECTDRAWSURFACE target,
    SniperShipObject *ship,const SniperShipObject *local,SniperShipObject *parent,uint32_t *blink,
    int detail,int mode,int32_t upper,int32_t lower,int32_t bonus,int32_t cx,int32_t cy){
    p->target=target;p->result=DD_OK;
    sniper_ship_render(ship,local,parent,blink,detail,mode,upper,lower,bonus,cx,cy,effect,body,text,p);
    p->target=NULL;return p->result;
}

typedef struct {
    SniperShipPainter *painter;LPDIRECTDRAWSURFACE target;
    SniperShipDrawOrder *order;const SniperShipPaintOptions *options;HRESULT result;
} OrderedPaint;
static void ordered_paint(void *context,SniperShipObject *ship,int32_t x,int32_t y){
    OrderedPaint *p=context;const SniperShipPaintOptions *o=p->options;
    int32_t id=ship->word[0x6c/4];
    SniperShipObject *parent=id>=0 && id<1024?o->players[id]:NULL;
    HRESULT hr=sniper_ship_paint(p->painter,p->target,ship,p->order->local,parent,o->blink,
        o->low_detail,o->label_mode,o->upper,o->lower,o->bonus,x,y);
    if(FAILED(hr) && SUCCEEDED(p->result))p->result=hr;
}
HRESULT sniper_ships_paint(SniperShipPainter *p,LPDIRECTDRAWSURFACE target,
    SniperShipDrawOrder *order,const SniperShipPaintOptions *options){
    OrderedPaint context={p,target,order,options,DD_OK};
    SniperOrderedShipDraw saved_draw=order->draw;void *saved_user=order->user;
    order->draw=ordered_paint;order->user=&context;sniper_ship_draw_order(order);
    order->draw=saved_draw;order->user=saved_user;return context.result;
}
