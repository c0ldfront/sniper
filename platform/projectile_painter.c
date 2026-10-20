#include "projectile_painter.h"
#include <string.h>
void sniper_projectile_painter_destroy(SniperProjectilePainter *p){
    for(int i=0;i<5;++i)sniper_surface_destroy(&p->assets[i]);
}
HRESULT sniper_projectile_painter_create(SniperProjectilePainter *p,LPDIRECTDRAW dd,HINSTANCE instance){
    memset(p,0,sizeof *p);
    for(int i=0;i<5;++i){
        HRESULT hr=sniper_surface_create(dd,instance,&sniper_projectile_assets[i],&p->assets[i]);
        if(FAILED(hr)){sniper_projectile_painter_destroy(p);return hr;}
    }
    return DD_OK;
}
static void draw(void *context,int asset,int32_t x,int32_t y,int32_t frame,int center,int opaque){
    SniperProjectilePainter *p=context;
    HRESULT hr=sniper_surface_draw(&p->assets[asset],p->target,x,y,frame,center,opaque);
    if(FAILED(hr) && SUCCEEDED(p->result))p->result=hr;
}
HRESULT sniper_projectiles_paint(SniperProjectilePainter *p,LPDIRECTDRAWSURFACE target,
    SniperProjectile *const *items,int32_t count,int32_t cx,int32_t cy,int suppress){
    p->target=target;p->result=DD_OK;
    for(int32_t i=0;i<count;++i)sniper_projectile_draw(items[i],cx,cy,suppress,draw,p);
    p->target=NULL;return p->result;
}
