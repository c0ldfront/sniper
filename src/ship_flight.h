#ifndef SNIPER_SHIP_FLIGHT_H
#define SNIPER_SHIP_FLIGHT_H
#include "flight.h"
#include "ship_create.h"
/* Applies the ordinary flight slice to the shared ship and its effect ring.
 * Input is a stable snapshot. Sound calls are returned in effects for delivery
 * by the host; callbacks that mutate flight state are not modeled here.
 * exhaust_ticks is global42833c, speed_exempt is global42930c>0.
 * fragment_count is initialized in/out weapon scratch, optionally NULL.
 * The ship's effect-ring cursor must be in0..11. */
void sniper_ship_flight(SniperShipObject *,const SniperFlightInput *,uint32_t,
                        const SniperBoostConfig *,int32_t,int,int,int32_t *,
                        SniperFlightEffects *,int32_t *fragment_count);
/* Polls keys in original order. Callback samples keyboard state only; it must
 * not inspect intermediate flight state or mutate ship/input state. */
void sniper_ship_flight_polled(SniperShipObject *,const SniperFlightInput *,uint32_t,
                        const SniperBoostConfig *,int32_t,int,int,int32_t *,
                        SniperFlightEffects *,int32_t *,SniperFlightKeyState,void *);
/* Audio callback can inspect the authoritative shared ship/exhaust timer at
 * each original sound call; it must not mutate simulation state. Effects remain
 * populated for diagnostics but must not be replayed after this API returns. */
typedef void (*SniperShipFlightAudio)(void *,int slot,int play);
void sniper_ship_flight_polled_audio(SniperShipObject *,const SniperFlightInput *,uint32_t,
                        const SniperBoostConfig *,int32_t,int,int,int32_t *,
                        SniperFlightEffects *,int32_t *,SniperFlightKeyState,void *,
                        SniperShipFlightAudio,void *);
#endif
