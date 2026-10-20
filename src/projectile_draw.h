#ifndef SNIPER_PROJECTILE_DRAW_H
#define SNIPER_PROJECTILE_DRAW_H
#include "projectile_update.h"
#include "sprite.h"
/* Asset slots: bullets,bombs,bullet impact,bomb explosion,repeller. */
extern const SniperSprite sniper_projectile_assets[5];
typedef void (*SniperProjectileDraw)(void *,int asset,int32_t x,int32_t y,int32_t frame,int centered,int opaque);
/*412e50; suppress_impacts is original422280 option. */
void sniper_projectile_draw(const SniperProjectile *,int32_t camera_x,int32_t camera_y,
    int suppress_impacts,SniperProjectileDraw,void *);
#endif
