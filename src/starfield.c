#include "starfield.h"
#include <stddef.h>
static int32_t random_offset(uint32_t *state)
{
    *state=*state*214013u+2531011u;
    return (int32_t)((*state>>16)&32767u)%640;
}
void sniper_starfield_generate(SniperStarfield *s,uint32_t *rng)
{
    int i=0;
    for(int x=-1920;x<18240;x+=320)
        for(int y=-1920;y<18240;y+=320,++i){
            s->near_layer[i].x=(int16_t)(x+random_offset(rng)-320);
            s->near_layer[i].y=(int16_t)(y+random_offset(rng)-320);
            s->far_layer[i].x=(int16_t)(x+random_offset(rng)-320);
            s->far_layer[i].y=(int16_t)(y+random_offset(rng)-320);
        }
}
static int32_t add(int32_t a,int32_t b){return (int32_t)((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return (int32_t)((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return (int32_t)((uint32_t)a*(uint32_t)b);}
int sniper_starfield_draw(const SniperStarfield *s,uint8_t *pixels,int32_t pitch,
    int32_t width,int32_t height,int32_t camera_x,int32_t camera_y,
    uint8_t near_color,uint8_t far_color)
{
    if(!s||!pixels||width<=0||height<=0||pitch<width)return -1;
    int32_t hw=width/2,hh=height/2,cx=add(camera_x,hw),cy=add(camera_y,hh);
    int32_t bounds[2][4];
    for(int layer=0;layer<2;++layer){
        int32_t d=layer?5:3,offset=layer?384:640;
        int32_t x=add(cx,mul(sub(offset,hw),d));
        int32_t y=add(cy,mul(sub(offset,hh),d));
        bounds[layer][0]=x/320;bounds[layer][1]=add(add(x,mul(width,d)),320)/320;
        bounds[layer][2]=y/320;bounds[layer][3]=add(add(y,mul(height,d)),320)/320;
        /* Restrict to complete generated table cells, before writing pixels. */
        if(bounds[layer][0]<0||bounds[layer][1]>=63||bounds[layer][2]<0||bounds[layer][3]>=63)return -1;
    }
    for(int layer=0;layer<2;++layer){
        const SniperStar *table=layer?s->far_layer:s->near_layer;
        int32_t d=layer?5:3;uint8_t color=layer?far_color:near_color;
        for(int32_t x=bounds[layer][0];x<=bounds[layer][1];++x)
            for(int32_t y=bounds[layer][2];y<=bounds[layer][3];++y){
                const SniperStar *p=&table[x*63+y];
                int32_t sx=add(hw,sub(p->x,cx)/d),sy=add(hh,sub(p->y,cy)/d);
                if(sx>=0&&sy>=0&&sx<width&&sy<height)
                    pixels[(size_t)sy*(size_t)pitch+(size_t)sx]=color;
            }
    }
    return 0;
}
