#ifndef SNIPER_PLAYER_REGISTRY_H
#define SNIPER_PLAYER_REGISTRY_H
#include "player_team.h"
#include "notices.h"
typedef struct {
    SniperShipObject **players,**ordered,*local;
    int32_t count,selected,dirty;
    SniperNotices *notices;
    const SniperShipDefaults *defaults;
    uint32_t milliseconds;
    SniperShipObject *(*allocate)(void *);
    void (*release)(void *,SniperShipObject *);
    void *user;
    uint32_t (*clock_ms)(void *);
    void *clock_user;
} SniperPlayerRegistry;
/*404340. Valid ID0..254,ordered capacity256 and valid constructor strings required.
 * Caller must enforce these bounds; native index255 aliases a palette global.
 * Failure returnsNULL without inserting (original insertsNULL and can crash).
 * Local pointer is not assigned here;caller establishes it after initial add.
 * Name must fit the notice buffer (original stack formatting is unbounded). */
SniperShipObject *sniper_player_add(SniperPlayerRegistry *,const SniperShipIdentity *);
/*404600. Does not clear local pointer;caller owns that lifecycle transition. */
void sniper_player_remove(SniperPlayerRegistry *,int32_t id);
#endif
