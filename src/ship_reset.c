#include "ship_reset.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s; memcpy(&s,&u,4); return s; }
static int32_t add(int32_t a,int32_t b) { return bits((uint32_t)a+(uint32_t)b); }
void sniper_ship_reset(int32_t s[0x110/4],const SniperShipDefaults *d) {
    static const unsigned clear[]={0x88,0x8c,0x94,0x98,0x9c,0x104,0x10c,0x108,
        0xb4,0xb8,0xb0,0x90,0xd8,0xdc,0xd4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc,0x100,0xa0};
    for (unsigned i=0;i<sizeof clear/sizeof *clear;++i) s[clear[i]/4]=0;
    s[0xc4/4]=d->recharge;s[0xc8/4]=d->energy_maximum;s[0xcc/4]=d->rotation;
    s[0xd0/4]=1;s[0xe0/4]=d->acceleration;s[0xe4/4]=d->speed;
    switch(s[0x40/4]) {
    case 0:
        s[0xe0/4]=add(d->acceleration,bits((uint32_t)d->acceleration_bonus*3u)/2);
        s[0xcc/4]=add(d->rotation,d->rotation_bonus);break;
    case 1:
        s[0xc4/4]=add(d->recharge,d->recharge_bonus);
        s[0xe4/4]=add(d->speed,d->speed_bonus);break;
    case 2:
        s[0xec/4]=1;s[0xc8/4]=add(d->energy_maximum,d->energy_bonus/2);break;
    case 3:
        s[0xd4/4]=1;s[0xc8/4]=add(d->energy_maximum,d->energy_bonus);break;
    }
    s[0x5c/4]=s[0xc8/4]>100000 ? s[0xc8/4] : 100000;
}
