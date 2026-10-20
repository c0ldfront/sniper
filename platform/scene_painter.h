#ifndef SNIPER_SCENE_PAINTER_H
#define SNIPER_SCENE_PAINTER_H
#include "actor_painter.h"
#include "map_painter.h"
#include "starfield_painter.h"
#include "hud_panel_painter.h"
#include "../src/scene_frame.h"
typedef struct {
    SniperSceneFrame *frame;
    LPDIRECTDRAWSURFACE target,primary;
    int32_t width,height,tick;
    const int32_t *camera_x,*camera_y;
    const uint8_t *map;const SniperStarfield *stars;
    uint8_t near_color,far_color;
    SniperMapPainter *map_painter;SniperActorLayers *actors;
    SniperShipDrawOrder *ship_order;const SniperShipPaintOptions *ship_options;
    SniperOverlayPainter *overlay;SniperProjectilePainter *projectiles;SniperShipPainter *ships;
    SniperHudPanels *hud_panels;SniperHudStatus *hud;const SniperNotices *notices;
    int suppress_impacts;uint32_t hud_background,hud_highlight;
    /* Special/radar/chat/mode handlers must draw immediately;no queued blits.
     * NULL causes E_NOTIMPL if an unrecovered stage is enabled. */
    HRESULT (*extension)(void *,SniperSceneStage);
    /* Optional presentation seam;otherwise uses primary->Flip(NULL,1).
     * Recovery implements403e40 when supplied;absence ends lost frame. */
    uint32_t (*present)(void *);
    int (*recover)(void *);
    void *user;
    HRESULT result;
} SniperScenePainter;
/* Borrowed components. Does not clear target (outside recovered frame slice).
 * Uses immediate sprite path and empty batch flush. Errors other than SURFACELOST prevent presentation
 * and return first failing HRESULT; lost draws reach native Flip recovery. Restores frame callback/context bindings. */
HRESULT sniper_scene_paint(SniperScenePainter *);
#endif
