#ifndef SNIPER_SHIP_ENERGY_H
#define SNIPER_SHIP_ENERGY_H
#include <stdint.h>
/* Ability order is original object offsets 0x88,0x8c,0x90,0x94.
 * Names remain unassigned until the input/configuration paths are verified. */
typedef struct {
    int32_t energy, recharge, maximum;
    int32_t ability[4];
} SniperShipEnergy;
/* Called only for the local player's live update path. Delta uses 10 ms ticks. */
void sniper_ship_energy_step(SniperShipEnergy *ship, int32_t delta,
                             const int32_t cost_per_tick[4]);
#endif
