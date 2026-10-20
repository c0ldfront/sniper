#ifndef SNIPER_MANUAL_WARP_H
#define SNIPER_MANUAL_WARP_H
#include "ship_world.h"
/*40577a..40585d: Insert requires exact energy==capacity. On success play the
 * warp sound before zeroing energy and spawn with reset0/move1. On rejection,
 * push the original300-tick notice. Returns1 on warp,0 on rejection. */
int sniper_manual_warp(SniperShipWorld *,SniperShipObject *);
#endif
