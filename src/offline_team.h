#ifndef SNIPER_OFFLINE_TEAM_H
#define SNIPER_OFFLINE_TEAM_H
#include "ship_world.h"
/* Offline411600 ->404530(local ID0). Dense order,selected index and list dirty
 * are shared with the player-list UI. Uses actual reset/attachment handlers.
 * World.milliseconds supplies the original clock read inside spawn/reset. */
void sniper_offline_team(SniperShipWorld *,SniperShipObject **ordered,
    int32_t *selected,int32_t *list_dirty,int32_t frequency);
#endif
