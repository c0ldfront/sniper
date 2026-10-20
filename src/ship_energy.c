#include "ship_energy.h"
#include <string.h>
/* Preserve x86 low-32-bit arithmetic without signed C overflow. */
static int32_t signed_bits(uint32_t bits) {
    int32_t value;
    memcpy(&value, &bits, sizeof value);
    return value;
}
/* Original 0x414376..0x41443c, including early epilogue at 0x414431.
 * Recharge is capped before costs. Insufficient energy disables an ability
 * without charging it; equality pays the cost and leaves the ability enabled. */
void sniper_ship_energy_step(SniperShipEnergy *s, int32_t delta,
                             const int32_t costs[4]) {
    s->energy = signed_bits((uint32_t)s->energy +
                            (uint32_t)s->recharge * (uint32_t)delta);
    if (s->energy > s->maximum) s->energy = s->maximum;
    for (unsigned i = 0; i < 4; ++i) {
        if (!s->ability[i]) continue;
        int32_t cost = signed_bits((uint32_t)delta * (uint32_t)costs[i]);
        if (cost > s->energy) s->ability[i] = 0;
        else s->energy = signed_bits((uint32_t)s->energy - (uint32_t)cost);
    }
}
