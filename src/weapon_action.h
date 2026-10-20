#ifndef SNIPER_WEAPON_ACTION_H
#define SNIPER_WEAPON_ACTION_H
#include "ship_create.h"
#include "local_config.h"
#include "projectile_update.h"
#include "notices.h"
typedef struct {
    SniperShipObject *local;
    const SniperLocalConfig *config;
    const uint8_t *map;
    SniperProjectile **projectiles;
    int32_t count,local_id,cooldown,safe_timer,position_dirty;
    SniperNotices *notices;
    void (*weapon)(void *,const int32_t args[14]);
    void *user;
} SniperWeaponActionWorld;
/*406431..406922. Input selection and cooldown subtraction occur before this
 * block. action0 guns,1 bombs,2..6 ability toggles,7 repeller. Returns type. */
int sniper_weapon_action(SniperWeaponActionWorld *,int active,int32_t action,int mine_modifier);
/* In/out dispatch fields. Caller initializes fields[1..5] to explicit scratch
 * values;original leaves those stack slots unchanged without selected fire.
 * Guns/bombs/repels overwrite them even if firing is later suppressed.
 * NULL uses deterministic zero scratch (legacy wrapper behavior). */
int sniper_weapon_action_fields(SniperWeaponActionWorld *,int active,int32_t action,int mine_modifier,int32_t fields[6]);
#endif
