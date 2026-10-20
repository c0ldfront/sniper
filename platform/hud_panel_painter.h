#ifndef SNIPER_HUD_PANEL_PAINTER_H
#define SNIPER_HUD_PANEL_PAINTER_H
#include "hud_painter.h"
#include "../src/hud_panel.h"
typedef struct {
    SniperTeamPanel team;SniperEquipmentPanel equipment;
    LPDIRECTDRAW dd;SniperSpriteSurface *fonts;LPDIRECTDRAWSURFACE target;
    HRESULT result;
} SniperHudPanels;
/* dd/fonts are borrowed;cached surfaces are owned. Initializes empty caches. */
void sniper_hud_panels_init(SniperHudPanels *,LPDIRECTDRAW,SniperSpriteSurface fonts[4]);
void sniper_hud_panels_destroy(SniperHudPanels *);
HRESULT sniper_hud_panels_paint(SniperHudPanels *,LPDIRECTDRAWSURFACE,SniperShipObject *,int32_t width,int32_t height,uint32_t background,uint32_t highlight);
/* Complete407c00 ordering: dynamic status,team/bounty,equipment. */
HRESULT sniper_hud_paint(SniperHudPanels *,LPDIRECTDRAWSURFACE,SniperHudStatus *,uint32_t background,uint32_t highlight);
#endif
