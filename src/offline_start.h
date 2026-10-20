#ifndef SNIPER_OFFLINE_START_H
#define SNIPER_OFFLINE_START_H
#include "ship_world.h"
typedef struct {
    SniperShipObject *local;
    int32_t local_id,frequency,watched,selected,list_dirty;
    int32_t height,history_size,force_four;
    uint32_t cursor_tick;
    void (*cursor)(void *,int32_t x,int32_t y);
    void *user;
} SniperOfflineStart;
/*403703..40383b after offline backend returns local ID0. Sorts the shared
 * dense list,forces selected index0,and spawns local if signed type<4.
 * Otherwise selects first non-spectator;offline watch callback is a no-op.
 * Caller supplies initialized registry/world and an accessible spawn map.
 * Spawn reads world.milliseconds;cursor_ms is the later independent read. */
void sniper_offline_start(SniperShipWorld *,SniperShipObject **ordered,
    SniperOfflineStart *,uint32_t cursor_ms);
#endif
