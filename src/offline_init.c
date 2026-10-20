#include "offline_init.h"
#include "level.h"
SniperShipObject *sniper_offline_init(SniperPlayerRegistry *registry,SniperLocalConfig *config,
    uint8_t *map,const SniperTileSize sizes[256],const uint8_t *level,size_t bytes,
    uint32_t generation_ms,uint32_t *generation_tick,const char *name,
    int32_t type,int32_t team,int32_t field70) {
    *generation_tick=generation_ms/10u;
    size_t count=sniper_level_record_count(bytes);
    for(size_t i=0;i<count;++i) {
        SniperTile t=sniper_tile_decode(level+i*4);
        if(sniper_map_place(map,sizes,t.x,t.y,t.tile))return NULL;
    }
    sniper_local_config(config);
    SniperShipDefaults defaults;sniper_local_ship_defaults(config,&defaults);
    const SniperShipDefaults *previous=registry->defaults;registry->defaults=&defaults;
    SniperShipIdentity identity={0,name,"",type,0,0,0,team,-1,field70};
    SniperShipObject *ship=sniper_player_add(registry,&identity);
    registry->defaults=previous;
    return ship;
}
