#ifndef SNIPER_CHAT_DISPATCH_H
#define SNIPER_CHAT_DISPATCH_H
#include "notices.h"
typedef struct {
    SniperShipObject *local, **ordered;
    int32_t count, selected, team_modulus, team_capacity;
    int32_t *requested_team;
    SniperNotices *notices;
    void (*send)(void *,int32_t recipient,const char *text);
    void (*team)(void *,int32_t frequency);
    void (*attach)(void *,int32_t target);
    void *user;
} SniperChatDispatch;
/*409b10. Caller supplies a NUL-terminated editor/macro string. Callbacks are
 * synchronous; team may modify local attachment before the following check.
 * Negative selected indexes and invalid IDIV inputs safely return, unlike
 * original invalid memory reads/divide exceptions. Uses startup C locale. */
void sniper_chat_dispatch(SniperChatDispatch *,const char *);
#endif
