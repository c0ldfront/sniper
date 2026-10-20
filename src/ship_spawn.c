#include "ship_spawn.h"
uint32_t sniper_random(uint32_t *state) {
    *state=*state*214013u+2531011u;
    return (*state>>16)&32767u;
}
void sniper_ship_spawn(int32_t s[0x110/4],const SniperShipDefaults *defaults,
    const uint8_t *map,int32_t players,uint32_t milliseconds,uint32_t *random_state,
    int reset,int move,int32_t *dirty) {
    s[0x60/4]=0;s[0x64/4]=0;s[0x74/4]=(int32_t)(milliseconds/10u);
    if (move) {
        int32_t width=players*1024/96+256;
        if (width>=1024) width=1024;
        int32_t x,y;unsigned occupied;
        do {
            x=(1024-width)/2+(int32_t)(sniper_random(random_state)%(uint32_t)(width-2))+1;
            y=(1024-width)/2+(int32_t)(sniper_random(random_state)%(uint32_t)(width-2))+1;
            occupied=0;
            for (int dy=-1;dy<=1;++dy)
                for(int dx=-1;dx<=1;++dx) occupied+=map[(y+dy)*1024+x+dx];
        } while(occupied);
        s[0x48/4]=x*16000+8000;s[0x4c/4]=y*16000+8000;
        s[0x50/4]=0;s[0x54/4]=0;
        s[0x58/4]=(int32_t)(sniper_random(random_state)%40u)*1000;
        s[0xbc/4]=48;*dirty=1;
    }
    if(reset) sniper_ship_reset(s,defaults);
}
