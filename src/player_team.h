#ifndef SNIPER_PLAYER_TEAM_H
#define SNIPER_PLAYER_TEAM_H
#include "ship_create.h"
/*40a1c0 default C-locale comparison:local player,teammates,others;then
 * case-insensitive byte names. Names must terminate within original63 bytes. */
int sniper_player_compare(const SniperShipObject *,const SniperShipObject *,const SniperShipObject *local);
/* Original CRT416930/416a90 pointer sort,including equal-name permutations. */
void sniper_player_sort(SniperShipObject **,int32_t count,const SniperShipObject *local);
typedef struct {
    SniperShipObject **players,**ordered,*local;
    int32_t count,selected,dirty,reset_on_team;
    void (*detach)(void *,int32_t);
    void (*spawn)(void *,SniperShipObject *,int reset,int move);
    void *user;
} SniperPlayerTeam;
/*404530. ID must index players;selected must be nonnegative (may exceed count).
 * Missing target still sorts/invalidates and may detach local from that ID. */
void sniper_player_team(SniperPlayerTeam *,int32_t id,int32_t team);
#endif
