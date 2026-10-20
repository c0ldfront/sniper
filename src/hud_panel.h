#ifndef SNIPER_HUD_PANEL_H
#define SNIPER_HUD_PANEL_H
#include "ship_create.h"
#include <stdint.h>
typedef uintptr_t SniperPanelSurface;
typedef struct {
    void (*release)(void *,SniperPanelSurface);
    SniperPanelSurface (*create)(void *,int32_t width,int32_t height);
    void (*color_key)(void *,SniperPanelSurface,uint32_t);
    void (*clear)(void *,SniperPanelSurface,uint32_t);
    void (*text)(void *,SniperPanelSurface,const char *,int32_t x,int32_t y,int color);
    void (*blit)(void *,SniperPanelSurface,int32_t x,int32_t y,int32_t width,int32_t height);
    void *user;
} SniperPanelOps;
typedef struct {
    SniperShipObject *local;
    SniperPanelSurface surface;
    uint32_t cached_sum,background;
    int32_t screen_width;
    SniperPanelOps ops;
} SniperTeamPanel;
/*407f9c..408158. Rebuilds on missing surface or changed team+bounty SUM.
 * Returns -1 on create failure rather than original null dereference. */
int sniper_team_panel_draw(SniperTeamPanel *);
typedef struct {
    SniperTeamPanel cache; /* shares lifecycle/text operations;screen_width unused */
    int32_t screen_height;
    uint32_t highlight;
    void (*rect)(void *,SniperPanelSurface,int32_t,int32_t,int32_t,int32_t,uint32_t);
    void (*transparent_text)(void *,SniperPanelSurface,const char *,int32_t,int32_t,int);
} SniperEquipmentPanel;
/*408158..4086ef. Separate text paths preserve416090 vs415cf0 calls. */
int sniper_equipment_panel_draw(SniperEquipmentPanel *);
#endif
