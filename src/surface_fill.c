#include "surface_fill.h"
#include <stddef.h>
#include <string.h>
#define LOST UINT32_C(0x887601c2)
#define BUSY UINT32_C(0x8876021c)
#define INVALID UINT32_C(0x80070057)
uint32_t sniper_surface_fill(const SniperFillSurface *s,int backbuffer,int32_t screen_width,const SniperFillRect *r,uint32_t color)
{
    uint32_t result;
    if(backbuffer&&screen_width==320){
        do{result=s->blt(s->user,r,color);}while(result==BUSY);
        if(result==LOST)return s->restore(s->user);
        return result;
    }
    SniperFillBuffer b={0};result=s->lock(s->user,&b);
    if(result)return result;
    if(!b.pixels||b.pitch<b.width||r->left<0||r->right<r->left||r->right>b.width||r->top<0||r->top>b.height||r->bottom>b.height){
        s->unlock(s->user,b.pixels);return INVALID;
    }
    for(int32_t y=r->top;y<r->bottom;++y)
        memset(b.pixels+(size_t)y*(size_t)b.pitch+(size_t)r->left,(uint8_t)color,(size_t)(r->right-r->left));
    return s->unlock(s->user,b.pixels);
}
static int32_t add(int32_t a,int32_t b){return (int32_t)((uint32_t)a+(uint32_t)b);}
uint32_t sniper_surface_vertical(const SniperFillSurface *s,int backbuffer,int32_t width,int32_t x,int32_t y,int32_t length,uint32_t color)
{
    SniperFillRect r={x,y,add(x,1),add(y,length)};
    return sniper_surface_fill(s,backbuffer,width,&r,color);
}
uint32_t sniper_surface_horizontal(const SniperFillSurface *s,int backbuffer,int32_t width,int32_t x,int32_t y,int32_t length,uint32_t color)
{
    SniperFillRect r={x,y,add(x,length),add(y,1)};
    return sniper_surface_fill(s,backbuffer,width,&r,color);
}
