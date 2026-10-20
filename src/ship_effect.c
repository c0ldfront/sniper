#include "ship_effect.h"
#include "flight_math.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s;memcpy(&s,&u,4);return s; }
static void emit(SniperShipObject *o,int32_t dx,int32_t dy,int32_t life,int32_t kind) {
    int32_t *s=o->word;
    unsigned offset=0x154/4+(unsigned)s[0x214/4]*4;
    s[offset]=bits((uint32_t)s[0x48/4]-(uint32_t)dx)/1000;
    s[offset+1]=bits((uint32_t)s[0x4c/4]-(uint32_t)dy)/1000;
    s[offset+2]=life;s[offset+3]=kind;
    s[0x214/4]=(s[0x214/4]+1)%12;
}
void sniper_ship_exhaust(SniperShipObject *o) {
    int32_t dx=0,dy=0;
    /* Same angle constant, x87 trig and CRT truncation as flight acceleration;
     * particle appears14 pixels behind quantized ship heading. */
    sniper_flight_accelerate(o->word[0x58/4],1,14000,1,&dx,&dy);
    emit(o,dx,dy,57,0);
}
void sniper_ship_effect1(SniperShipObject *o) { emit(o,0,0,52,1); }
