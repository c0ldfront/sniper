#include "attachment_command.h"
typedef struct {SniperShipObject *players[1024];SniperNotices *notices;int32_t *trace;int n;} Context;
static void attach(void *user,int32_t target){Context *c=user;c->trace[c->n++]=3;c->trace[c->n++]=target;c->trace[c->n++]=c->players[0]->word[0x5c/4];sniper_attachment(c->players,0,c->notices,0,target);}
static void release(void *user){Context *c=user;c->trace[c->n++]=1;}
static void sound(void *user){Context *c=user;c->trace[c->n++]=2;c->trace[c->n++]=c->players[0]->word[0x5c/4];}
int test_attachment_command(SniperShipObject ships[8],SniperNotices *notices,int32_t selected,int32_t trace[8]) {
    Context c={0};SniperShipObject *ordered[8];for(int i=0;i<8;++i)c.players[i]=ordered[i]=ships+i;
    c.notices=notices;c.trace=trace;
    SniperAttachmentCommand command={ships,ordered,8,selected,0,notices,attach,release,sound,&c};
    sniper_attachment_command(&command);return c.n;
}
