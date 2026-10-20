#ifndef SNIPER_ATTACHMENT_H
#define SNIPER_ATTACHMENT_H
#include "ship_create.h"
typedef struct {
    int32_t ticks[4];
    char text[4][128];
    int32_t color[4];
} SniperAttachmentHud;
/*404fe0: names are original null-terminated31-byte ship names. Valid IDs0..1023.
 * Null lookup entry is ignored. Target equals local ID emits a HUD notice. */
void sniper_attachment(SniperShipObject *const players[1024],int32_t local_id,
    SniperAttachmentHud *,int32_t player,int32_t target);
#endif
