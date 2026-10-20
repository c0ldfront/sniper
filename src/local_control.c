#include "local_control.h"
#include <string.h>
int sniper_local_control(SniperLocalControl *c)
{
    if(c->local->word[0x5c/4]<0){c->key=0;c->repeat=0;c->character=0;return 0;}
    if(c->key==123)c->damage(c->user,c->local,0x27ac41,c->local_id);
    memset(c->joystick,0,sizeof c->joystick);
    if(c->joystick_enabled){
        c->joystick[0]=52;c->joystick[1]=0xcc3;c->joystick[10]=65535;
        c->poll_joystick(c->user,0,c->joystick);
    }
    for(int stage=0;stage<3;++stage)c->stage(c->user,stage);
    c->key=0;c->repeat=0;c->character=0;c->mouse_x_event=0;c->mouse_y_event=0;
    return 1;
}
