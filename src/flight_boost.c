#include "flight_boost.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s; memcpy(&s,&u,4); return s; }
int sniper_flight_boost(int32_t type,int32_t ticks,int32_t *energy,
                        const SniperBoostConfig *c,int32_t *acceleration,int32_t *maximum) {
    int32_t cost=bits((uint32_t)c->cost*(uint32_t)ticks);
    if (cost>=*energy) return 0;
    *energy=bits((uint32_t)*energy-(uint32_t)cost);
    *acceleration=c->acceleration;
    *maximum=c->maximum;
    if (type==0) *acceleration=bits((uint32_t)*acceleration+(uint32_t)c->type0_acceleration_bonus);
    if (type==1) *maximum=bits((uint32_t)*maximum+(uint32_t)c->type1_speed_bonus);
    return 1;
}
