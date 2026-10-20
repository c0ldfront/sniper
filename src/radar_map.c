#include "radar_map.h"
#include <string.h>
#include <limits.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return bits((uint32_t)a*(uint32_t)b);}
int sniper_radar_map_size(int32_t width,int32_t scale,int small,int32_t *out) {
    int32_t n=bits((uint32_t)width<<3);
    if(!scale||(n==INT_MIN&&scale==-1))return 0;
    n/=scale;if(small)n=(int32_t)(((uint32_t)n*6u)>>3);
    *out=n;return 1;
}
void sniper_radar_view(int32_t width,int32_t height,int32_t x,int32_t y,
    int32_t mw,int32_t mh,int small,SniperRadarView *out) {
    int32_t size=mul(width/24,4);if(small)size=mul(size,6)/8;
    x=sub(mul(add(width/2,x)/16,mw)/1024,size/2);
    y=sub(mul(add(height/2,y)/16,mh)/1024,size/2);
    if(x<=0)x=0;
    if(x>=sub(mw,size))x=sub(mw,size);
    if(y<=0)y=0;
    if(y>=sub(mh,size))y=sub(mh,size);
    *out=(SniperRadarView){x,y,size,sub(sub(width,size),1),sub(sub(height,size),1)};
}
int sniper_radar_map_pixels(uint8_t *out,size_t bytes,int32_t pitch,int32_t width,int32_t height,
    const uint8_t map[1048576],uint8_t wall,uint8_t safe) {
    if(!out||!map||width<=0||height<=0||width>4096||height>4096||pitch<width)return 0;
    if((uint64_t)(height+1)*(uint32_t)pitch+(uint32_t)width+1>bytes)return 0;
    for(int y=0;y<1024;++y) {
        size_t row=(size_t)(1+y*height/1024)*(uint32_t)pitch+1;
        for(int x=0;x<1024;++x) {
            uint8_t tile=map[y*1024+x];
            if(tile>=1&&tile<=250)out[row+(size_t)(x*width/1024)]=tile==152?safe:wall;
        }
    }
    return 1;
}
