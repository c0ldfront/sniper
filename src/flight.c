#include "flight.h"
#include "flight_math.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s; memcpy(&s,&u,4); return s; }
static void sound(SniperFlightEffects *e,int slot,int play,const SniperFlight *s,SniperFlightAudio audio,void *audio_user) {
    int n=e->sound_count++;e->sound_slot[n]=slot;e->sound_play[n]=play;
    if(audio)audio(audio_user,s,slot,play);
}
static void apply(SniperFlight *s,uint32_t buttons,int boost_pressed,
                  const SniperBoostConfig *boost,int32_t ticks,int engine_sound,
                  int speed_exempt,SniperFlightEffects *e,int32_t *fragment_count,
                  SniperFlightAudio audio,void *audio_user) {
    int32_t acceleration=s->acceleration,maximum=s->maximum;
    if (e->thrust) {
        if (boost_pressed || (buttons & 4)) {
            if (engine_sound) sound(e,2,1,s,audio,audio_user);
            e->boosted=sniper_flight_boost(s->type,ticks,&s->energy,boost,&acceleration,&maximum);
        } else if (engine_sound) {
            sound(e,1,1,s,audio,audio_user);sound(e,2,0,s,audio,audio_user);
        }
        if (fragment_count) *fragment_count=bits((uint32_t)acceleration*(uint32_t)e->thrust*(uint32_t)ticks);
        sniper_flight_accelerate(s->control.heading,e->thrust,acceleration,ticks,&s->vx,&s->vy);
        s->exhaust_ticks=bits((uint32_t)s->exhaust_ticks-(uint32_t)ticks);
        if (s->exhaust_ticks<=0) {
            s->exhaust_ticks=bits((uint32_t)s->exhaust_ticks+8u);
            e->exhaust=1;
        }
    } else {
        s->exhaust_ticks=0;
        if (engine_sound) sound(e,0,1,s,audio,audio_user);
        sound(e,1,0,s,audio,audio_user);sound(e,2,0,s,audio,audio_user);
    }
    if (!speed_exempt) {
        if (fragment_count) *fragment_count=maximum;
        sniper_flight_limit(maximum,&s->vx,&s->vy);
    }
}

void sniper_flight_step_scratch(SniperFlight *s,const SniperFlightInput *in,uint32_t buttons,
                       const SniperBoostConfig *boost,int32_t ticks,int engine_sound,
                       int speed_exempt,SniperFlightEffects *e,int32_t *fragment_count) {
    memset(e,0,sizeof *e);
    e->thrust=sniper_flight_input(&s->control,in,ticks);
    apply(s,buttons,(in->keys & SNIPER_SHIFT)!=0,boost,ticks,engine_sound,
          speed_exempt,e,fragment_count,0,0);
}
void sniper_flight_step_polled_audio(SniperFlight *s,const SniperFlightInput *in,uint32_t buttons,
                       const SniperBoostConfig *boost,int32_t ticks,int engine_sound,
                       int speed_exempt,SniperFlightEffects *e,int32_t *fragment_count,
                       SniperFlightKeyState key,void *user,SniperFlightAudio audio,void *audio_user) {
    memset(e,0,sizeof *e);
    e->thrust=sniper_flight_input_polled(&s->control,in,ticks,key,user);
    int boost_pressed=e->thrust && (key(user,16)&0x8000);
    apply(s,buttons,boost_pressed,boost,ticks,engine_sound,speed_exempt,e,fragment_count,audio,audio_user);
}
void sniper_flight_step_polled(SniperFlight *s,const SniperFlightInput *in,uint32_t buttons,
                       const SniperBoostConfig *boost,int32_t ticks,int engine_sound,
                       int speed_exempt,SniperFlightEffects *e,int32_t *fragment_count,
                       SniperFlightKeyState key,void *user) {
    sniper_flight_step_polled_audio(s,in,buttons,boost,ticks,engine_sound,speed_exempt,e,fragment_count,key,user,0,0);
}

void sniper_flight_step(SniperFlight *s,const SniperFlightInput *in,uint32_t buttons,
                       const SniperBoostConfig *boost,int32_t ticks,int engine_sound,
                       int speed_exempt,SniperFlightEffects *e) {
    sniper_flight_step_scratch(s,in,buttons,boost,ticks,engine_sound,speed_exempt,e,0);
}
