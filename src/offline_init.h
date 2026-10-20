#ifndef SNIPER_OFFLINE_INIT_H
#define SNIPER_OFFLINE_INIT_H
#include "player_registry.h"
#include "local_config.h"
#include "map.h"
#include <stddef.h>
/*410ff0 map/config/player composition after file bytes are acquired. Does not
 * clear caller's map/registry or assign local pointer (original caller does).
 * Registry.milliseconds supplies ship constructor time;generation time is a
 * separate read. Config is initialized after all complete level records.
 * ReturnsNULL on invalid map input/allocation failure;earlier records remain
 * applied. Original unchecked failure/crash behavior is not reproduced. */
SniperShipObject *sniper_offline_init(SniperPlayerRegistry *,SniperLocalConfig *,
    uint8_t *map,const SniperTileSize sizes[256],const uint8_t *level,size_t bytes,
    uint32_t generation_ms,uint32_t *generation_tick,const char *name,
    int32_t type,int32_t team,int32_t field70);
#endif
