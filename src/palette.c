#include "palette.h"
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int sniper_palette_resource(SniperPaletteEntry out[256],const uint8_t *dib,size_t size){
    for(unsigned i=0;i<256;++i)out[i]=(SniperPaletteEntry){((i>>5)&7)*255/7,((i>>2)&7)*255/7,(i&3)*255/3,0};
    if(!dib)return 0;
    if(size<40)return -1;
    uint32_t header=le32(dib);unsigned bits=dib[14]|(unsigned)dib[15]<<8;
    if(header<40 || header>size)return -1;
    if(bits>8)return 0;
    uint32_t count=le32(dib+32);if(!count)count=1u<<bits;
    if(count>256 || count>(size-header)/4)return -1;
    for(uint32_t i=0;i<count;++i){const uint8_t *p=dib+header+4*i;out[i]=(SniperPaletteEntry){p[2],p[1],p[0],0};}
    return 0;
}
