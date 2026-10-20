#ifndef SNIPER_WEAPON_CONTROL_H
#define SNIPER_WEAPON_CONTROL_H
#include "weapon_action.h"
#include "weapon_input.h"
#include "position_dispatch.h"
/* Shared ship required. Transfers dirty state action->dispatch->action.
 * Six in/out fields are original outgoing locals,even on cancelled fire.
 * Initialize fields[1..5] explicitly;NULL selects zero scratch. Original
 * untouched stack data is not recreated with uninitialized C memory.
 * Return indicates backend dispatch,not successful weapon construction. */
int sniper_weapon_action_dispatch(SniperWeaponActionWorld *,SniperPositionDispatch *,int active,int32_t action,int mine_modifier,int32_t fields[6]);
int sniper_weapon_control(SniperWeaponActionWorld *,SniperPositionDispatch *,const SniperWeaponInput *,int32_t ticks,int32_t fields[6]);
/* Input and action only, for callers owning the later dispatch stage. */
int sniper_weapon_control_fields(SniperWeaponActionWorld *,const SniperWeaponInput *,int32_t ticks,int32_t fields[6]);
#endif
