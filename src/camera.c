#include "camera.h"
#include "ship_spawn.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
SniperCamera sniper_camera(const SniperShipObject *local,const SniperShipObject *watched,
    int32_t width,int32_t height,int special,int32_t tx,int32_t ty,int32_t shake,uint32_t *rng) {
    SniperCamera c={0,0};
    if(special) {
        c.x=bits(((uint32_t)tx<<4)-(uint32_t)(width/2)+8u);
        c.y=add(bits((uint32_t)height*2u)/-3,bits(((uint32_t)ty<<4)+8u));
        return c;
    }
    const SniperShipObject *ship=local->word[0x40/4]==4 ? watched:local;
    if(ship) {
        c.x=bits((uint32_t)(ship->word[0x48/4]/1000)-(uint32_t)(width/2));
        c.y=bits((uint32_t)(ship->word[0x4c/4]/1000)-(uint32_t)(height/2));
    }
    if(shake>0) {
        int32_t span=shake/4+4;
        c.x=add(c.x,(int32_t)(sniper_random(rng)%(uint32_t)span)-span/2);
        c.y=add(c.y,(int32_t)(sniper_random(rng)%(uint32_t)span)-span/2);
    }
    return c;
}
