#ifndef SNIPER_OFFLINE_COMBAT_H
#define SNIPER_OFFLINE_COMBAT_H
#include "ship_world.h"
#include "projectile_frame.h"
#include "projectile_create.h"
#include "projectile_explosion.h"
#include "ship_damage.h"
#include "death_event.h"
#include "weapon_create.h"
#include "weapon_action.h"
#include "weapon_input.h"
typedef struct {
    SniperShipWorld *ships;
    SniperShipObject **ordered_ships;
    SniperProjectile *projectiles[512];
    int32_t count,next_group,shake,repel_timer;
    int32_t weapon_cooldown,safe_timer;
    int32_t bullet_sounds[4],bomb_sounds[4],repel_sound,impact_sound,explosion_sound,death_sound;
    SniperProjectile *(*allocate)(void *);
    void (*release)(void *,SniperProjectile *);
    void (*sound)(void *,int32_t handle,int32_t x,int32_t y);
    void *user;
    SniperProjectileWorld *active_update; /* private during frame */
} SniperOfflineCombat;
/* Requires offline local player ID0; shares ship RNG/config/map/HUD/coverage.
 * Caller owns memory and sound. Construct does not append or increment group. */
void sniper_offline_combat_construct(SniperOfflineCombat *,SniperProjectile *,const int32_t args[15]);
void sniper_offline_combat_damage(SniperOfflineCombat *,SniperShipObject *,int32_t amount,int32_t owner);
void sniper_offline_combat_frame(SniperOfflineCombat *,const int32_t *ticks);
void sniper_offline_combat_weapon(SniperOfflineCombat *,const int32_t args[14]);
int sniper_offline_combat_action(SniperOfflineCombat *,int active,int32_t action,int mine_modifier);
int sniper_offline_combat_input(SniperOfflineCombat *,const SniperWeaponInput *,int32_t ticks);
#endif
