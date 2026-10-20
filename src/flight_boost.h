#ifndef SNIPER_FLIGHT_BOOST_H
#define SNIPER_FLIGHT_BOOST_H
#include <stdint.h>
typedef struct {
    int32_t cost, acceleration, maximum, type0_acceleration_bonus, type1_speed_bonus;
} SniperBoostConfig;
/* Original 0x406186..0x4061d5. Invoke only when thrust and boost are requested.
 * Outputs must initially contain the ship's normal acceleration and maximum.
 * Return indicates whether the cost was paid. Equality is insufficient. */
int sniper_flight_boost(int32_t type,int32_t ticks,int32_t *energy,
                        const SniperBoostConfig *,int32_t *acceleration,int32_t *maximum);
#endif
