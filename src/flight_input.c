#include "flight_input.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s; memcpy(&s,&u,4); return s; }
int32_t sniper_flight_input(SniperFlightControl *s,const SniperFlightInput *in,int32_t ticks) {
    int32_t rate=(in->keys & SNIPER_SHIFT) ? 66 : s->rotation_rate;
    int32_t thrust=0,turn=0;
    if (s->lock_ticks>0) rate=33;
    else if (s->attached_id==-1) {
        if (in->keys & SNIPER_UP) thrust=1;
        if (in->keys & SNIPER_DOWN) thrust=-1;
        if (in->joystick_enabled && !thrust) {
            if (in->joystick_y<0x4000) thrust=1;
            if (in->joystick_y>0xc000) thrust=-1;
        }
    }
    if (in->keys & SNIPER_LEFT) turn=bits(0u-(uint32_t)rate);
    if (in->keys & SNIPER_RIGHT) turn=rate;
    if (in->joystick_enabled && !turn) {
        int32_t axis=bits(in->joystick_x-0x8000u)/0x2aaa;
        if (axis>2) axis=2;
        if (axis< -2) axis=-2;
        int32_t product=bits((uint32_t)rate*(uint32_t)axis);
        /* Original adds one for negative values then arithmetic-shifts.
         * Division matches except INT_MAX+1 is not involved here. */
        turn=product/2;
    }
    s->heading=bits((uint32_t)turn*(uint32_t)ticks+(uint32_t)s->heading+40000u)%40000;
    return thrust;
}

int32_t sniper_flight_input_polled(SniperFlightControl *s,const SniperFlightInput *in,
                                  int32_t ticks,SniperFlightKeyState key,void *user) {
    SniperFlightInput snapshot=*in;
    snapshot.keys=(key(user,16)&0x8000)?SNIPER_SHIFT:0;
    if (s->lock_ticks<=0 && s->attached_id==-1) {
        if (key(user,38)&0x8000) snapshot.keys|=SNIPER_UP;
        if (key(user,40)&0x8000) snapshot.keys|=SNIPER_DOWN;
    }
    if (key(user,37)&0x8000) snapshot.keys|=SNIPER_LEFT;
    if (key(user,39)&0x8000) snapshot.keys|=SNIPER_RIGHT;
    return sniper_flight_input(s,&snapshot,ticks);
}
