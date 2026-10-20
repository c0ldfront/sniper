#ifndef SNIPER_SHIP_DAMAGE_H
#define SNIPER_SHIP_DAMAGE_H
#include "ship_create.h"
#include "local_config.h"
typedef struct {
    const SniperLocalConfig *config;
    const uint8_t *map;
    int32_t local_id;
    int32_t (*random)(void *);
    void (*sound)(void *,int32_t x,int32_t y);
    void (*death)(void *,int32_t killer,int32_t bounty);
    void (*message)(void *,const char *,int32_t duration,int32_t color);
    void *user;
} SniperDamageWorld;
/* 414b10. Effect cursor must be0..11; original probability divisor must be
 * nonzero. Callbacks retain backend/audio/HUD ordering. */
void sniper_ship_damage(SniperShipObject *,SniperDamageWorld *,int32_t amount,int32_t owner);
#endif
