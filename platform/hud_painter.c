#include "hud_painter.h"
#include "../src/font.h"
typedef struct {
    SniperHudStatus *hud;SniperSpriteSurface *fonts;LPDIRECTDRAWSURFACE target;
    SniperFillSurface fill;HRESULT result;
    SniperHudClock milliseconds,last_receive;void *user;
} Painter;
static void remember(Painter *p,HRESULT hr){if(FAILED(hr)&&SUCCEEDED(p->result))p->result=hr;}
static void glyph(void *ctx,int color,int32_t x,int32_t y,int32_t frame)
{
    Painter *p=ctx;
    if(color<0||color>=4){remember(p,E_INVALIDARG);return;}
    remember(p,sniper_surface_draw(&p->fonts[color],p->target,x,y,frame,0,0));
}
static void text(void *ctx,const char *s,int32_t x,int32_t y,int color){sniper_font_draw(s,x,y,color,glyph,ctx);}
static void rect(void *ctx,int32_t l,int32_t t,int32_t r,int32_t b,uint32_t color)
{
    Painter *p=ctx;SniperFillRect bounds={l,t,r,b};
    remember(p,(HRESULT)sniper_surface_fill(&p->fill,1,p->hud->width,&bounds,color));
}
static void line(void *ctx,int32_t x,int32_t y,int32_t length,uint32_t color)
{
    Painter *p=ctx;remember(p,(HRESULT)sniper_surface_vertical(&p->fill,1,p->hud->width,x,y,length,color));
}
static uint32_t now(void *ctx){Painter *p=ctx;return p->milliseconds(p->user);}
static uint32_t last(void *ctx){Painter *p=ctx;return p->last_receive(p->user);}
HRESULT sniper_hud_status_paint(SniperHudStatus *h,SniperSpriteSurface fonts[4],LPDIRECTDRAWSURFACE target)
{
    if(!h||!h->local||!fonts||!target)return E_INVALIDARG;
    if(h->show_energy_bar&&h->maximum_energy<=0)return E_INVALIDARG;
    if(h->connected&&h->player_count>1&&(!h->milliseconds||!h->last_receive_tick))return E_INVALIDARG;
    Painter p={.hud=h,.fonts=fonts,.target=target,.result=DD_OK,
        .milliseconds=h->milliseconds,.last_receive=h->last_receive_tick,.user=h->user};
    SniperLabelDraw saved_text=h->text;SniperHudRect saved_rect=h->rect;SniperHudLine saved_line=h->vertical_line;
    sniper_fill_surface_bind(&p.fill,target);
    h->text=text;h->rect=rect;h->vertical_line=line;h->milliseconds=now;h->last_receive_tick=last;h->user=&p;
    sniper_hud_status(h);
    h->text=saved_text;h->rect=saved_rect;h->vertical_line=saved_line;
    h->milliseconds=p.milliseconds;h->last_receive_tick=p.last_receive;h->user=p.user;
    return p.result;
}
