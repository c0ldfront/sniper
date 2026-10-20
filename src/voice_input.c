#include "voice_input.h"
int sniper_voice_input(SniperVoiceInput *c,int32_t key,int32_t shift) {
    if(!(shift&0x8000)||(key!=0x7a&&key!=0x7b))return 0;
    if(!c->enabled||c->selected<0||c->selected>=c->count)return 1;
    SniperShipObject *target=c->ordered[c->selected];
    if(!target->word[0x70/4]) {
        sniper_notice_push(c->notices,"Player is not receiving audio messages.",400,2);
        return 1;
    }
    if(c->transport_present&&c->large_transfer_count)return 1;
    int slot=key-0x7a;
    c->send(c->user,target->word[0x44/4],slot,c->recordings[slot],c->lengths[slot]);
    return 1;
}
