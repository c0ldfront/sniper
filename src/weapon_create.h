#ifndef SNIPER_WEAPON_CREATE_H
#define SNIPER_WEAPON_CREATE_H
#include "projectile_frame.h"
typedef struct {
    SniperProjectile **items;
    int32_t *count,*next_group;
    int32_t speed,spread;
    SniperProjectile *(*create)(void *,const int32_t args[15]);
    SniperProjectileStep update;
    SniperProjectileRelease release;
    void *user;
} SniperWeaponCreateWorld;
/*409d70. args:type,level,fragmentType,fragmentLevel,fragmentCount,multifire,
 * elapsed,heading,x,y,vx,vy,owner,team. count>=510 rejects whole event.
 * Zero elapsed skips update. Original allocation failure can append NULL. */
void sniper_weapon_create(SniperWeaponCreateWorld *,const int32_t args[14]);
#endif
