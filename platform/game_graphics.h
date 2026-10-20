#ifndef SNIPER_GAME_GRAPHICS_H
#define SNIPER_GAME_GRAPHICS_H
#include "game_display.h"
#include "text_panel_painter.h"
#include "radar_painter.h"
#include "chat_painter.h"
#include "editor_painter.h"
#include "../src/statistics_panel.h"
#include "scene_painter.h"
typedef struct {
    LPDIRECTDRAWPALETTE palette;
    SniperMapPainter map;
    SniperShipPainter ships;
    SniperProjectilePainter projectiles;
    SniperOverlayPainter overlay;
    SniperHudPanels panels;
    uint8_t colors[16];
    SniperTextPanelPainter help,statistics;
    SniperStatisticsPanel statistics_content;
    SniperEditorPainter editor;
    SniperRadarPainter radar;
    SniperChatPainter chat;
} SniperGameGraphics;
/* Resource-backed graphics for an open8-bit display. Zero initialize before
 * first call;owns all sprite/palette/cache references. Display is borrowed.
 * Existing painters each own their fonts (separate copies,not original shared
 * allocation layout). External bitmap overrides remain outside this loader. */
HRESULT sniper_game_graphics_open(SniperGameGraphics *,SniperGameDisplay *,HINSTANCE);
void sniper_game_graphics_close(SniperGameGraphics *);
#endif
