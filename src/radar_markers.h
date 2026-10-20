#ifndef SNIPER_RADAR_MARKERS_H
#define SNIPER_RADAR_MARKERS_H
#include "radar_map.h"
#include "ship_create.h"
#include "pickup_collect.h"
#include "projectile_update.h"
typedef struct {
    SniperShipObject *local,*reference,**ships;
    SniperPickupObject **pickups;
    SniperProjectile **projectiles;
    int32_t ship_count,pickup_count,projectile_count,local_id,watched_id;
    int32_t screen_width,map_width,map_height,tick,special,tile_x,tile_y;
    int32_t bounty_upper,bounty_lower,bounty_bonus;
    uint32_t friendly,enemy_positive,enemy_nonpositive,projectile_color,pickup_color;
    SniperRadarView view;
    void (*rect)(void *,int32_t left,int32_t top,int32_t right,int32_t bottom,uint32_t color);
    void *user;
} SniperRadarMarkers;
/*4075d9..407a6f after successful map blit. reference is players[local_id],or
 * players[watched_id] when local type4;may beNULL. Emits complete rectangles
 * with original anchor-only crop test (far edges may extend past crop). */
void sniper_radar_markers(const SniperRadarMarkers *);
#endif
