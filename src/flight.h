#ifndef SNIPER_FLIGHT_H
#define SNIPER_FLIGHT_H
#include "flight_input.h"
#include "flight_boost.h"
typedef struct {
    SniperFlightControl control;
    int32_t type, energy, acceleration, maximum, vx, vy, exhaust_ticks;
} SniperFlight;
typedef struct {
    int32_t thrust, boosted, exhaust;
    /* Original sound calls in their original order. 0x40aae0=play,
     * 0x40aab0=stop. Slots0/1/2 map globals0x4218fc/1900/1904. */
    int32_t sound_count, sound_slot[3], sound_play[3];
} SniperFlightEffects;
/* Synchronous sound seam at original instruction order; state is read-only.
 * Called after steering, before boost/acceleration and speed limiting. */
typedef void (*SniperFlightAudio)(void *,const SniperFlight *,int slot,int play);
/* Original flight segment excluding weapon input. speed_exempt is global
 * 0x42930c>0. joystick_buttons must be zero if joystick was not polled.
 * Effects describe external calls; caller must implement sound and exhaust. */
void sniper_flight_step(SniperFlight *,const SniperFlightInput *,uint32_t joystick_buttons,
                       const SniperBoostConfig *,int32_t ticks,int engine_sound,
                       int speed_exempt,SniperFlightEffects *);
/* Preserve the stack scratch later consumed as weapon fragment count.
 * With speed limiting it becomes effective maximum; exempt thrust writes
 * wrapped acceleration*thrust*ticks; exempt idle leaves the input unchanged.
 * Caller initializes this optional in/out value; earlier provenance is unknown. */
void sniper_flight_step_scratch(SniperFlight *,const SniperFlightInput *,uint32_t,
                               const SniperBoostConfig *,int32_t,int,int,
                               SniperFlightEffects *,int32_t *fragment_count);
/* Live key polling, including a separate Shift read for boosting only when
 * thrust is nonzero. Same callback-state restrictions as flight_input_polled. */
void sniper_flight_step_polled(SniperFlight *,const SniperFlightInput *,uint32_t,
                              const SniperBoostConfig *,int32_t,int,int,
                              SniperFlightEffects *,int32_t *,SniperFlightKeyState,void *);
void sniper_flight_step_polled_audio(SniperFlight *,const SniperFlightInput *,uint32_t,
                              const SniperBoostConfig *,int32_t,int,int,
                              SniperFlightEffects *,int32_t *,SniperFlightKeyState,void *,
                              SniperFlightAudio,void *);
#endif
