#ifndef SNIPER_OFFLINE_SIMULATION_H
#define SNIPER_OFFLINE_SIMULATION_H
#include "offline_combat.h"
#include "offline_pickups.h"
/*4021bf..402338. Input/backend generation precede this stage; camera/render
 * follow it. pickups may be NULL only for a world with no pickups. If present,
 * it must be bound to combat->ships. Does not advance wall/frame clocks. */
void sniper_offline_simulation(SniperOfflineCombat *,SniperOfflinePickups *,const int32_t *ticks);
#endif
