#ifndef SNIPER_CAPABILITY_INPUT_H
#define SNIPER_CAPABILITY_INPUT_H
#include "ship_create.h"
/* End/Shift-End: X-Radar/ghosting; Home/Shift-Home: stealth/cloak;
 * Delete: multifire. shift_state is the signed or unsigned GetKeyState result.
 * Returns1 for a recognized key, including an unavailable capability. */
int sniper_capability_input(SniperShipObject *,int32_t key,int32_t shift_state,int32_t *position_dirty);
#endif
