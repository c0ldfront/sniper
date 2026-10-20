#include "spectator_control.h"
#include <string.h>
void sniper_spectator_control(SniperSpectatorControl *s) {
    uint32_t delta=(uint32_t)s->tick-(uint32_t)s->last_tick;int32_t elapsed;memcpy(&elapsed,&delta,4);
    if(elapsed>=400){const int32_t args[13]={-1,-1,0,0,0,0,0,0,0,0,0,0,0};
        s->position(s->user,args);s->last_tick=s->tick;}
    if((s->input->key==9||s->input->key==17)&&s->selected>=0&&s->selected<s->count) {
        SniperShipObject *ship=s->ordered[s->selected];
        if(ship->word[0x48/4]!=-1&&ship->word[0x40/4]<4) {
            s->watched=ship->word[0x44/4];s->watch(s->user,s->watched);
        }
    }
    s->input->key=0;s->input->repeat=0;s->input->character=0;
    s->input->mouse_x_event=0;s->input->mouse_y_event=0;
}
