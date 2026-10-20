#ifndef SNIPER_PRIZE_H
#define SNIPER_PRIZE_H
#include "ship_create.h"
#include "local_config.h"
typedef struct {
    const SniperLocalConfig *config;
    int32_t positive_sound,negative_sound,warp_sound;
    int32_t (*random)(void *);
    void (*spawn)(void *,SniperShipObject *,int reset,int move);
    void (*sound)(void *,int32_t handle);
    void (*message)(void *,const char *,int32_t duration,int32_t color);
    void *user;
} SniperPrizeWorld;
/* Full415030. Positive codes1..21 and all nonpositive codes accepted.
 * Returns-1 for invalid positive codes (original dereferences a null string).
 * Random must return0..32767 and timed-prize divisors must be positive. */
int sniper_prize(SniperShipObject *,SniperPrizeWorld *,int32_t code);
#endif
