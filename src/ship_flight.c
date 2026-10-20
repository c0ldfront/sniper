#include "ship_flight.h"
#include "ship_effect.h"
typedef struct {
    SniperShipObject *ship;
    int32_t *exhaust_ticks;
    SniperShipFlightAudio callback;
    void *user;
} AudioContext;
static void sound_event(void *user,const SniperFlight *state,int slot,int play){
    AudioContext *c=user;
    /* At all native sound sites,only heading and idle exhaust timer may have
     * changed. Energy,velocity and effect ring still retain their entry values. */
    c->ship->word[0x58/4]=state->control.heading;
    *c->exhaust_ticks=state->exhaust_ticks;
    c->callback(c->user,slot,play);
}
static void flight(SniperShipObject *ship,const SniperFlightInput *input,
                        uint32_t buttons,const SniperBoostConfig *boost,
                        int32_t ticks,int engine_sound,int speed_exempt,
                        int32_t *exhaust_ticks,SniperFlightEffects *effects,
                        int32_t *fragment_count,SniperFlightKeyState key,void *user,
                        SniperShipFlightAudio audio,void *audio_user) {
    int32_t *s=ship->word;
    SniperFlight flight={
        {s[0x58/4],s[0xcc/4],s[0x104/4],s[0x6c/4]},
        s[0x40/4],s[0x5c/4],s[0xe0/4],s[0xe4/4],
        s[0x50/4],s[0x54/4],*exhaust_ticks
    };
    AudioContext context={ship,exhaust_ticks,audio,audio_user};
    if (key) sniper_flight_step_polled_audio(&flight,input,buttons,boost,ticks,engine_sound,
                                      speed_exempt,effects,fragment_count,key,user,
                                      audio?sound_event:0,&context);
    else sniper_flight_step_scratch(&flight,input,buttons,boost,ticks,engine_sound,
                                    speed_exempt,effects,fragment_count);
    s[0x58/4]=flight.control.heading;
    s[0x5c/4]=flight.energy;
    s[0x50/4]=flight.vx;
    s[0x54/4]=flight.vy;
    *exhaust_ticks=flight.exhaust_ticks;
    /* The original emits before speed limiting. The emitter reads only
     * heading,position and ring state, so these operations commute. */
    if (effects->exhaust) sniper_ship_exhaust(ship);
}

void sniper_ship_flight(SniperShipObject *ship,const SniperFlightInput *input,
                        uint32_t buttons,const SniperBoostConfig *boost,
                        int32_t ticks,int engine_sound,int speed_exempt,
                        int32_t *exhaust_ticks,SniperFlightEffects *effects,
                        int32_t *fragment_count) {
    flight(ship,input,buttons,boost,ticks,engine_sound,speed_exempt,
           exhaust_ticks,effects,fragment_count,0,0,0,0);
}
void sniper_ship_flight_polled(SniperShipObject *ship,const SniperFlightInput *input,
                        uint32_t buttons,const SniperBoostConfig *boost,
                        int32_t ticks,int engine_sound,int speed_exempt,
                        int32_t *exhaust_ticks,SniperFlightEffects *effects,
                        int32_t *fragment_count,SniperFlightKeyState key,void *user) {
    flight(ship,input,buttons,boost,ticks,engine_sound,speed_exempt,
           exhaust_ticks,effects,fragment_count,key,user,0,0);
}

void sniper_ship_flight_polled_audio(SniperShipObject *ship,const SniperFlightInput *input,
                        uint32_t buttons,const SniperBoostConfig *boost,
                        int32_t ticks,int engine_sound,int speed_exempt,
                        int32_t *exhaust_ticks,SniperFlightEffects *effects,
                        int32_t *fragment_count,SniperFlightKeyState key,void *user,
                        SniperShipFlightAudio audio,void *audio_user) {
    flight(ship,input,buttons,boost,ticks,engine_sound,speed_exempt,
           exhaust_ticks,effects,fragment_count,key,user,audio,audio_user);
}
