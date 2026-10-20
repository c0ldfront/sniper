#include "ordinary_control.h"
typedef struct {SniperOrdinaryControl *world;SniperLocalControl original;} Context;
static void damage(void *user,SniperShipObject *ship,int32_t amount,int32_t owner) {
    Context *c=user;c->original.damage(c->original.user,ship,amount,owner);
}
static uint32_t poll(void *user,uint32_t id,uint32_t info[13]) {
    Context *c=user;return c->original.poll_joystick(c->original.user,id,info);
}
static void audio_event(void *user,int slot,int play){
    SniperOrdinaryControl *w=user;if(w->audio_effects){SniperFlightEffects e={0};e.sound_count=1;e.sound_slot[0]=slot;e.sound_play[0]=play;w->audio_effects(w->audio_user,&e);}
}
static void stage(void *user,int stage_id) {
    Context *c=user;SniperOrdinaryControl *w=c->world;
    SniperLocalControl *l=w->lifecycle;
    if(stage_id==0) {
        SniperBoostConfig boost;sniper_local_boost(w->weapons->config,&boost);
        SniperFlightInput input={0,(uint32_t)l->joystick_enabled,l->joystick[2],l->joystick[3]};
        sniper_ship_flight_polled_audio(l->local,&input,l->joystick[8],&boost,w->ticks,
            w->engine_sound,w->speed_exempt,&w->exhaust_ticks,&w->effects,
            &w->fields[4],w->input.key_state,w->input.user,audio_event,w);
    } else if(stage_id==1) {
        SniperWeaponInput input=w->input;
        input.joystick_enabled=l->joystick_enabled;
        input.joystick_buttons=(int32_t)l->joystick[8];
        input.joystick_pov=(int32_t)l->joystick[10];
        w->weapons->local=l->local;
        w->weapons->local_id=l->local_id;
        sniper_weapon_control_fields(w->weapons,&input,w->ticks,w->fields);
    } else {
        w->position->local=l->local;
        w->position->position_dirty=w->weapons->position_dirty;
        w->dispatched=sniper_position_dispatch(w->position,w->fields);
        w->weapons->position_dirty=w->position->position_dirty;
    }
}
int sniper_ordinary_control(SniperOrdinaryControl *w) {
    Context c={w,*w->lifecycle};
    w->lifecycle->damage=damage;w->lifecycle->poll_joystick=poll;
    w->lifecycle->stage=stage;w->lifecycle->user=&c;
    int result=sniper_local_control(w->lifecycle);
    w->lifecycle->damage=c.original.damage;
    w->lifecycle->poll_joystick=c.original.poll_joystick;
    w->lifecycle->stage=c.original.stage;w->lifecycle->user=c.original.user;
    return result;
}
