#include "offline_team.h"
#include "player_team.h"
static void detach(void *user,int32_t target) {sniper_ship_world_offline_attach(user,target);}
static void spawn(void *user,SniperShipObject *ship,int reset,int move) {
    SniperShipWorld *w=user;SniperShipDefaults defaults;
    sniper_local_ship_defaults(&w->config,&defaults);
    sniper_ship_spawn(ship->word,&defaults,w->movement.map,w->player_count,
        sniper_ship_world_clock(w),&w->random_state,reset,move,&w->position_dirty);
}
void sniper_offline_team(SniperShipWorld *w,SniperShipObject **ordered,
    int32_t *selected,int32_t *list_dirty,int32_t frequency) {
    SniperPlayerTeam team={w->players,ordered,w->players[0],w->player_count,
        *selected,*list_dirty,w->config.word[(0x4282ad-0x4281c9)/4],detach,spawn,w};
    sniper_player_team(&team,0,frequency);
    *selected=team.selected;*list_dirty=team.dirty;
}
