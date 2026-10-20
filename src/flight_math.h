#ifndef SNIPER_FLIGHT_MATH_H
#define SNIPER_FLIGHT_MATH_H
#include <stdint.h>
/* Original x87 flight arithmetic. Uses current x87 precision mode.
 * Requires x86/x86-64 and GCC-compatible inline assembly. */
void sniper_flight_accelerate(int32_t heading, int32_t thrust, int32_t acceleration,
                              int32_t ticks, int32_t *vx, int32_t *vy);
void sniper_flight_limit(int32_t maximum, int32_t *vx, int32_t *vy);
#endif
