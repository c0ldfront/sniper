#ifndef SNIPER_ACTOR_PAINTER_H
#define SNIPER_ACTOR_PAINTER_H
#include "overlay_painter.h"
#include "projectile_painter.h"
#include "ship_painter.h"
#include "../src/actor_layers.h"
/* Uses caller-owned surfaces. Ordinary actor stage only; caller draws map,
 * HUD and notices separately in the full frame order. */
HRESULT sniper_actors_paint(SniperActorLayers *,SniperShipDrawOrder *,
    const SniperShipPaintOptions *,SniperOverlayPainter *,SniperProjectilePainter *,
    SniperShipPainter *,LPDIRECTDRAWSURFACE,int suppress_impacts);
#endif
