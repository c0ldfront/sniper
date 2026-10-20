#include "ship_create.h"
#include <string.h>
void sniper_ship_create_clocked(SniperShipObject *o,const SniperShipIdentity *id,
                        const SniperShipDefaults *defaults,uint32_t milliseconds,
                        uint32_t (*clock_ms)(void *),void *clock_user) {
    int32_t *s=o->word;char *raw=(char *)s;
    s[0x70/4]=id->field70;s[0x6c/4]=id->attached_id;
    raw[31]=0;
    /* strncpy semantics: zero-pad short names, truncate long names. */
    size_t n=0;while(n<31 && id->name[n]) ++n;
    memcpy(raw,id->name,n);memset(raw+n,0,31-n);
    memcpy(raw+32,id->squad,strlen(id->squad)+1);
    s[0x40/4]=id->type;s[0x44/4]=id->player_id;
    s[0x48/4]=-1;s[0x4c/4]=-1;s[0x50/4]=0;s[0x54/4]=0;
    s[0x78/4]=id->field78;s[0x7c/4]=id->field7c;s[0x80/4]=id->field80;
    s[0x68/4]=id->team;s[0xa4/4]=0;s[0xbc/4]=0;
    memset(raw+0x154,0,0xc0);s[0x214/4]=0;
    s[0x60/4]=0;s[0x64/4]=0;s[0xa8/4]=0;s[0xac/4]=0;
    s[0x74/4]=(int32_t)((clock_ms?clock_ms(clock_user):milliseconds)/10u);
    /* Constructor413786 then reset/spawn4137ce samples again,even move0. */
    s[0x74/4]=(int32_t)((clock_ms?clock_ms(clock_user):milliseconds)/10u);
    sniper_ship_reset(s,defaults);
}
void sniper_ship_create(SniperShipObject *o,const SniperShipIdentity *id,
                        const SniperShipDefaults *defaults,uint32_t milliseconds) {
    sniper_ship_create_clocked(o,id,defaults,milliseconds,NULL,NULL);
}
