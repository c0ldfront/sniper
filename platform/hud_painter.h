#ifndef SNIPER_HUD_PAINTER_H
#define SNIPER_HUD_PAINTER_H
#include "fill_painter.h"
#include "../src/hud_status.h"
/* Caller owns font surfaces and HUD state. Paints dynamic status only;
 * target is the frame backbuffer. Existing clock callbacks retain their user
 * context; draw callbacks are temporarily replaced and then restored. */
HRESULT sniper_hud_status_paint(SniperHudStatus *,SniperSpriteSurface fonts[4],LPDIRECTDRAWSURFACE);
#endif
