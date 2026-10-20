#include "attachment_command.h"
#include <stddef.h>
void sniper_attachment_command(SniperAttachmentCommand *c) {
    SniperShipObject *local=c->local;
    if(local->word[0x6c/4]!=-1){c->attach(c->user,-1);return;}
    for(int32_t i=0;i<c->count;++i)if(c->ordered[i]->word[0x6c/4]==c->local_id) {
        c->release_children(c->user);return;
    }
    const char *error=NULL;
    if(local->word[0x5c/4]!=local->word[0xc8/4])error="Not enough energy to attach.";
    else {
        if(c->selected<0 || c->selected>=c->count)return;
        SniperShipObject *target=c->ordered[c->selected];
        int32_t id=target->word[0x44/4];
        if(id==c->local_id)error="Cannot attach to self.";
        else if(target->word[0x68/4]!=local->word[0x68/4])error="Must attach to somebody of same frequency.";
        else if(target->word[0x40/4]==4)error="Cannot attach to a spectator.";
        else {
            if(target->word[0x6c/4]!=-1)id=target->word[0x6c/4];
            c->sound(c->user);
            local->word[0x5c/4]/=3;
            c->attach(c->user,id);
            return;
        }
    }
    sniper_notice_push(c->notices,error,300,2);
}
