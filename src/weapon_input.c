#include "weapon_input.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
SniperWeaponSelection sniper_weapon_input(const SniperWeaponInput *in,int32_t ticks,int32_t *cooldown){
    SniperWeaponSelection out={0,0};
    *cooldown=bits((uint32_t)*cooldown-(uint32_t)ticks);
    if(in->key_state(in->user,0x11)&0x8000){
        out.active=1;if(in->key_state(in->user,0x10)&0x8000)out.action=7;
    }
    if(in->joystick_enabled){
        if(in->joystick_buttons&1){out.active=1;out.action=0;}
        if(in->joystick_buttons&10){out.active=1;out.action=1;}
        switch(in->joystick_pov){
        case 0:out.active=1;out.action=6;break;
        case 18000:out.active=1;out.action=5;break;
        case 9000:out.active=1;out.action=3;break;
        case 27000:out.active=1;out.action=2;break;
        }
    }
    if(in->mouse_pressed && in->mouse_x<60){
        int32_t half=in->vertical_extent/2;
        if(in->mouse_y>half){
            int32_t slot=bits((uint32_t)in->mouse_y-(uint32_t)half)/8;
            if(slot<8){out.active=1;out.action=slot;}
        }
    }
    if(in->key_state(in->user,9)&0x8000){out.active=1;out.action=1;}
    return out;
}
