#include "attachment.h"
#include <string.h>
void sniper_attachment(SniperShipObject *const players[1024],int32_t local_id,
    SniperAttachmentHud *hud,int32_t player,int32_t target) {
    SniperShipObject *s=players[player];
    if(!s)return;
    s->word[0x6c/4]=target;
    if(target!=local_id)return;
    unsigned slot=0;
    while(slot<4 && hud->ticks[slot]>0)++slot;
    if(slot==4)slot=0;
    static const char prefix[]="Player Attached: ";
    const char *name=(const char *)s->word;
    memcpy(hud->text[slot],prefix,sizeof prefix-1);
    memcpy(hud->text[slot]+sizeof prefix-1,name,strlen(name)+1);
    hud->ticks[slot]=600;hud->color[slot]=2;
}
