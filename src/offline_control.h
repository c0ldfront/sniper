#ifndef SNIPER_OFFLINE_CONTROL_H
#define SNIPER_OFFLINE_CONTROL_H
#include "offline_combat.h"
#include "ordinary_control.h"
/* Ordinary local input bound to the real offline world. Caller supplies
 * lifecycle,position,input and persistent flight/scratch state in control.
 * control.weapons is temporarily bound internally (may be NULL on entry).
 * Ship and ID are bound to offline player0. Damage uses combat damage/death;
 * weapons use combat allocation/construction/update; position send is the
 * original offline no-op. All borrowed callbacks and weapons pointer restored.
 * Configuration,HUD,cooldown,safe timer and dirty state share combat storage.
 * Returns the lifecycle's ran/dead result,not whether a shot was fired. */
int sniper_offline_control(SniperOfflineCombat *,SniperOrdinaryControl *);
#endif
