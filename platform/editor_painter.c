#include "editor_painter.h"
#include <string.h>
typedef struct {
    SniperEditorPainter *p;SniperSpriteSurface *tiles;LPDIRECTDRAWSURFACE target;
    const SniperEditorRenderState *state;
    DDSURFACEDESC lock;int locked;HRESULT result;
} Paint;
HRESULT sniper_editor_painter_create(SniperEditorPainter *p,LPDIRECTDRAW draw,HINSTANCE instance,LPDIRECTDRAWPALETTE palette){
    const SniperSprite desc={159,0,0,16,16,1,1,0,1};
    memset(p,0,sizeof *p);HRESULT hr=sniper_surface_create(draw,instance,&desc,&p->cursor);
    if(SUCCEEDED(hr)&&palette){hr=IDirectDrawSurface_SetPalette(p->cursor.surface,palette);if(SUCCEEDED(hr))hr=sniper_surface_restore(&p->cursor);}
    if(FAILED(hr))sniper_editor_painter_close(p);
    return hr;
}
void sniper_editor_painter_close(SniperEditorPainter *p){sniper_surface_destroy(&p->cursor);}
static HRESULT palette(Paint *p,int32_t x,int32_t y){
    int32_t w=p->tiles->descriptor.width*p->tiles->descriptor.columns,h=p->tiles->descriptor.height*p->tiles->descriptor.rows;
    RECT src={0,0,w,h},dst={x,y,x+w,y+h};HRESULT hr;
    do{hr=IDirectDrawSurface_Blt(p->target,&dst,p->tiles->surface,&src,0,NULL);}while(hr==DDERR_WASSTILLDRAWING);
    if(hr==DDERR_SURFACELOST){sniper_surface_restore(p->tiles);IDirectDrawSurface_Restore(p->target);}
    return hr;
}
static void line(Paint *p,const SniperEditorDrawCommand *c){
    /* Native415c80/415cb0 write directly inside the existing lock. Bound unsafe
     * offscreen/negative native spans; valid spans retain exact endpoint rules. */
    if(!p->locked||c->length<=0)return;
    int64_t x=c->x,y=c->y,n=c->length;
    uint8_t *pixels=p->lock.lpSurface;int64_t w=p->lock.dwWidth,h=p->lock.dwHeight;
    if(c->kind==SNIPER_EDITOR_HORIZONTAL){
        if(y<0||y>=h)return;
        if(x<0){n+=x;x=0;}if(x>=w||n<=0)return;if(n>w-x)n=w-x;
        memset(pixels+y*p->lock.lPitch+x,(uint8_t)c->color,(size_t)n);
    }else{
        if(x<0||x>=w)return;
        if(y<0){n+=y;y=0;}if(y>=h||n<=0)return;if(n>h-y)n=h-y;
        for(int64_t i=0;i<n;++i)pixels[(y+i)*p->lock.lPitch+x]=(uint8_t)c->color;
    }
}
static int draw(void *user,const SniperEditorDrawCommand *c){
    Paint *p=user;HRESULT hr=DD_OK;
    if(c->kind==SNIPER_EDITOR_UNLOCK){
        if(p->locked){hr=IDirectDrawSurface_Unlock(p->target,p->lock.lpSurface);p->locked=0;}
    }else if(FAILED(p->result))return 0;
    else switch(c->kind){
        case SNIPER_EDITOR_CURSOR:hr=sniper_surface_draw(&p->p->cursor,p->target,c->x,c->y,0,0,0);break;
        case SNIPER_EDITOR_LOCK:
            memset(&p->lock,0,sizeof p->lock);p->lock.dwSize=sizeof p->lock;
            hr=IDirectDrawSurface_Lock(p->target,NULL,&p->lock,DDLOCK_WAIT,NULL);
            if(FAILED(hr))return 0; /* Original skips outline on lock failure. */
            p->locked=1;
            if(!p->lock.lpSurface||p->lock.lPitch<(LONG)p->lock.dwWidth||p->lock.ddpfPixelFormat.dwRGBBitCount!=8){
                IDirectDrawSurface_Unlock(p->target,p->lock.lpSurface);p->locked=0;p->result=DDERR_INVALIDPIXELFORMAT;return 0;
            }
            return 1;
        case SNIPER_EDITOR_HORIZONTAL:case SNIPER_EDITOR_VERTICAL:line(p,c);break;
        case SNIPER_EDITOR_RECTANGLE:{
            /* Original415f80 outlines an inclusive rectangle inside one lock;
             * it is not the half-open filled rectangle helper415e50. */
            memset(&p->lock,0,sizeof p->lock);p->lock.dwSize=sizeof p->lock;
            hr=IDirectDrawSurface_Lock(p->target,NULL,&p->lock,DDLOCK_WAIT,NULL);
            if(FAILED(hr))return 0;
            p->locked=1;
            if(!p->lock.lpSurface||p->lock.lPitch<(LONG)p->lock.dwWidth||p->lock.ddpfPixelFormat.dwRGBBitCount!=8){
                IDirectDrawSurface_Unlock(p->target,p->lock.lpSurface);p->locked=0;p->result=DDERR_INVALIDPIXELFORMAT;return 0;
            }
            SniperEditorDrawCommand edge={.kind=SNIPER_EDITOR_HORIZONTAL,.x=c->x,.y=c->y,
                .length=(int32_t)((uint32_t)c->right-(uint32_t)c->x+1u),.color=c->color};
            line(p,&edge);edge.y=c->bottom;line(p,&edge);
            edge.kind=SNIPER_EDITOR_VERTICAL;edge.y=c->y;edge.length=(int32_t)((uint32_t)c->bottom-(uint32_t)c->y);
            line(p,&edge);edge.x=c->right;line(p,&edge);
            hr=IDirectDrawSurface_Unlock(p->target,p->lock.lpSurface);p->locked=0;break;
        }
        case SNIPER_EDITOR_PALETTE:hr=palette(p,c->x,c->y);break;
    }
    if(FAILED(hr)&&SUCCEEDED(p->result))p->result=hr;
    return SUCCEEDED(hr);
}
HRESULT sniper_editor_paint(SniperEditorPainter *p,SniperSpriteSurface *tiles,LPDIRECTDRAWSURFACE target,const SniperEditorRenderState *state){
    if(!p||!p->cursor.surface||!tiles||!tiles->surface||!target||!state)return E_INVALIDARG;
    Paint paint={.p=p,.tiles=tiles,.target=target,.state=state,.result=DD_OK};
    sniper_editor_render(state,draw,&paint);
    if(paint.locked)IDirectDrawSurface_Unlock(target,paint.lock.lpSurface);
    return paint.result;
}
