#include "offline_start.h"
#include "player_team.h"
void sniper_offline_start(SniperShipWorld *world,SniperShipObject **ordered,
    SniperOfflineStart *s,uint32_t cursor_ms) {
    s->local_id=0;world->movement.local_id=0;s->local=world->players[0];
    s->frequency=s->local->word[0x68/4];s->watched=-1;s->list_dirty=1;
    sniper_player_sort(ordered,world->player_count,s->local);
    /* Original searches for old selected ID but discards the result. */
    s->selected=0;
    if(s->local->word[0x40/4]<4) {
        SniperShipDefaults defaults;sniper_local_ship_defaults(&world->config,&defaults);
        sniper_ship_spawn(s->local->word,&defaults,world->movement.map,world->player_count,
            sniper_ship_world_clock(world),&world->random_state,1,1,&world->position_dirty);
    } else {
        for(int32_t i=0;i<world->player_count;++i)if(ordered[i]->word[0x40/4]<4) {
            s->watched=ordered[i]->word[0x44/4];break;
        }
    }
    world->movement.watched_id=s->watched;
    int32_t limit=s->height/24;
    if(limit<s->history_size)s->history_size=limit;
    if(s->force_four)s->history_size=4;
    s->cursor_tick=(world->clock_ms?sniper_ship_world_clock(world):cursor_ms)/10u-10000u;
    s->cursor(s->user,0,0);
}
