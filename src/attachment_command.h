#ifndef SNIPER_ATTACHMENT_COMMAND_H
#define SNIPER_ATTACHMENT_COMMAND_H
#include "notices.h"
typedef struct {
    SniperShipObject *local,**ordered;
    int32_t count,selected,local_id;
    SniperNotices *notices;
    void (*attach)(void *,int32_t target);
    void (*release_children)(void *);
    void (*sound)(void *);
    void *user;
} SniperAttachmentCommand;
/* Unshifted F7,405986..405d0d. Caller handles Shift macro dispatch.
 * Negative selected indexes safely return instead of original invalid read. */
void sniper_attachment_command(SniperAttachmentCommand *);
#endif
