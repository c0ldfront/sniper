#ifndef SNIPER_SHIP_CREATE_H
#define SNIPER_SHIP_CREATE_H
#include "ship_reset.h"
/* Full object storage. Uninitialized original fields are intentionally preserved. */
typedef struct { int32_t word[0x218/4]; } SniperShipObject;
typedef struct {
    int32_t player_id;
    const char *name,*squad;
    int32_t type, field78, field7c, field80, team, attached_id, field70;
} SniperShipIdentity;
/* Squad must fit in31 bytes plus terminator. Original strcpy is unbounded;
 * caller validates this limit. Clock value represents final of two reads. */
void sniper_ship_create(SniperShipObject *,const SniperShipIdentity *,
                        const SniperShipDefaults *,uint32_t milliseconds);
void sniper_ship_create_clocked(SniperShipObject *,const SniperShipIdentity *,
    const SniperShipDefaults *,uint32_t milliseconds,uint32_t (*clock_ms)(void *),void *clock_user);
#endif
