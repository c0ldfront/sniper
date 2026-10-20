#ifndef SNIPER_EDITOR_PAINTER_H
#define SNIPER_EDITOR_PAINTER_H
#include "sprite_surface.h"
#include "fill_painter.h"
#include "../src/editor_render.h"
typedef struct {SniperSpriteSurface cursor;} SniperEditorPainter;
/* Owns edit.bmp/resource159 cursor. Tiles/palette are borrowed at paint time. */
HRESULT sniper_editor_painter_create(SniperEditorPainter *,LPDIRECTDRAW,HINSTANCE,LPDIRECTDRAWPALETTE);
void sniper_editor_painter_close(SniperEditorPainter *);
HRESULT sniper_editor_paint(SniperEditorPainter *,SniperSpriteSurface *tiles,
    LPDIRECTDRAWSURFACE,const SniperEditorRenderState *);
#endif
