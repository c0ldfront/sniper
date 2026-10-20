#ifndef SNIPER_TEXT_PANEL_PAINTER_H
#define SNIPER_TEXT_PANEL_PAINTER_H
#include "sprite_surface.h"
#include "../src/text_panel.h"
typedef struct {
    LPDIRECTDRAWSURFACE surface;
    SniperTextPanelLayout layout;
} SniperTextPanelPainter;
void sniper_text_panel_close(SniperTextPanelPainter *);
HRESULT sniper_text_panel_paint(SniperTextPanelPainter *,LPDIRECTDRAW,LPDIRECTDRAWSURFACE,
    SniperSpriteSurface fonts[4],const char *title,const SniperPanelRow *,int32_t count,
    int32_t per,int32_t x,int32_t y,uint8_t background,uint8_t border,
    int (*recover)(void *),void *user);
#endif
