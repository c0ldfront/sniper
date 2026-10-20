#ifndef SNIPER_SHIP_RESET_H
#define SNIPER_SHIP_RESET_H
#include <stdint.h>
/* Offset-indexed object storage retains fields whose meaning is not yet known.
 * This routine requires at least 0x110 bytes, with type at offset0x40. */
typedef struct {
    int32_t recharge, energy_maximum, rotation, acceleration, speed;
    int32_t rotation_bonus, acceleration_bonus, speed_bonus, energy_bonus, recharge_bonus;
} SniperShipDefaults;
void sniper_ship_reset(int32_t object[0x110/4],const SniperShipDefaults *);
#endif
