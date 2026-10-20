#ifndef SNIPER_SESSION_PAINTER_H
#define SNIPER_SESSION_PAINTER_H
#include "game_graphics.h"
#include "display_recovery_painter.h"
#include "../src/offline_session.h"
typedef struct {
    SniperOfflineSession *session;SniperGameDisplay *display;SniperGameGraphics *graphics;
    SniperSceneFrame frame;SniperScenePainter scene;
    SniperActorLayers actors;SniperShipDrawOrder order;SniperShipPaintOptions options;
    SniperHudStatus hud;SniperDisplayRecovery recovery;uint32_t blink;
} SniperSessionPainter;
/* Borrowed resources must outlive painter. Caller sets options/scene overlay
 * flags and handlers explicitly;unimplemented enabled overlays fail. */
void sniper_session_painter_init(SniperSessionPainter *,SniperOfflineSession *,SniperGameDisplay *,SniperGameGraphics *);
/* Clear backbuffer,advance session,paint and present. No window message pump
 * or audio playback;flight sound requests remain available on session. */
HRESULT sniper_session_present(SniperSessionPainter *);
#endif
