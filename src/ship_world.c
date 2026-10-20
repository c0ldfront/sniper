#include "ship_world.h"
#include "notices.h"
static SniperShipObject *lookup(void *context,int32_t id) {
    SniperShipWorld *w=context;
    return id>=0 && id<1024 ? w->players[id] : 0;
}
static void detach(void *context,int32_t id) {
    SniperShipWorld *w=context;
    if(w->offline_backend)sniper_ship_world_offline_attach(w,id);
    else w->detach(w->backend,id);
}
static int32_t random_value(void *context) {
    SniperShipWorld *w=context;
    return (int32_t)sniper_random(&w->random_state);
}
static void spawn(void *context,SniperShipObject *s,int reset,int move) {
    SniperShipWorld *w=context;
    SniperShipDefaults defaults;
    sniper_local_ship_defaults(&w->config,&defaults);
    sniper_ship_spawn(s->word,&defaults,w->movement.map,w->player_count,
        sniper_ship_world_clock(w),&w->random_state,reset,move,&w->position_dirty);
}
static void move(void *context,SniperShipObject *s,int32_t ticks,int effects) {
    SniperShipWorld *w=context;
    sniper_move_object(s->word,&w->movement,ticks,effects);
}
#define CFG(a) w->config.word[((a)-0x4281c9)/4]
void sniper_ship_world_update(SniperShipWorld *w,SniperShipObject *s,int32_t ticks) {
    SniperShipObject *local=lookup(w,w->movement.local_id);
    SniperShipUpdateWorld update={
        .local_id=w->movement.local_id,.respawn_delay=CFG(0x428219),
        .remote_energy=CFG(0x42824d),.jitter_period=CFG(0x4282a1),
        .jitter_radius=CFG(0x42829d),
        .ability_cost={CFG(0x428225),CFG(0x428221),CFG(0x428299),CFG(0x428229)},
        .milliseconds=s->word[0x44/4]!=w->movement.local_id && s->word[0x48/4]!=-1 ?
            sniper_ship_world_clock(w):w->milliseconds,
        .local_team=local ? local->word[0x68/4] : 0,
        .coverage=w->coverage,.coverage_stride=32,
        .lookup=lookup,.detach=detach,.spawn=spawn,.move=move,
        .random=random_value,.user=w
    };
    w->movement.bounce_divisor=sniper_local_bounce(&w->config);
    sniper_ship_update(s,&update,ticks);
}

static void prize_sound(void *context,int32_t handle) {
    SniperShipWorld *w=context;
    w->prize_sound(w->backend,handle);
}
static void prize_message(void *context,const char *text,int32_t duration,int32_t color) {
    SniperShipWorld *w=context;
    sniper_notice_push(&w->attachment_hud,text,duration,color);
    if(w->prize_message)w->prize_message(w->backend,text,duration,color);
}
int sniper_ship_world_prize(SniperShipWorld *w,SniperShipObject *s,int32_t code) {
    SniperPrizeWorld prize={&w->config,w->positive_sound,w->negative_sound,
        w->warp_sound,random_value,spawn,prize_sound,prize_message,w};
    return sniper_prize(s,&prize,code);
}

static void ordered_update(void *context,SniperShipObject *ship,int32_t ticks) {
    sniper_ship_world_update(context,ship,ticks);
}
void sniper_ship_world_update_all(SniperShipWorld *w,SniperShipObject **ordered,
    const int32_t *ticks) {
    sniper_ship_update_order(ordered,&w->player_count,ticks,ordered_update,w);
}

void sniper_ship_world_offline_attach(SniperShipWorld *w,int32_t target) {
    /* Offline virtual+2c (411620) always dispatches player0. */
    sniper_attachment(w->players,w->movement.local_id,&w->attachment_hud,0,target);
}
