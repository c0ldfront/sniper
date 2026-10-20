#ifndef SNIPER_HUD_STATUS_H
#define SNIPER_HUD_STATUS_H
#include "ship_create.h"
#include "ship_labels.h"
typedef void (*SniperHudRect)(void *,int32_t left,int32_t top,int32_t right,int32_t bottom,uint32_t color);
typedef void (*SniperHudLine)(void *,int32_t x,int32_t y,int32_t length,uint32_t color);
typedef uint32_t (*SniperHudClock)(void *);
typedef struct {
    SniperShipObject *local;
    int32_t width,height,show_energy_bar,maximum_energy,blink,safety_ticks,player_count,connected;
    uint32_t energy_high,energy_medium,energy_low,border,capacity;
    SniperLabelDraw text;
    SniperHudRect rect;
    SniperHudLine vertical_line;
    SniperHudClock milliseconds,last_receive_tick;
    void *user;
} SniperHudStatus;
/* Dynamic HUD stage407c00..407f9c. Cached team/bounty and equipment panels
 * follow separately. maximum_energy must be positive when drawing the bar. */
void sniper_hud_status(SniperHudStatus *);
#endif
