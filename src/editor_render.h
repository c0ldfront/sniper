#ifndef SNIPER_EDITOR_RENDER_H
#define SNIPER_EDITOR_RENDER_H
#include <stdint.h>
typedef struct {
 int32_t cursor_x,cursor_y,camera_x,camera_y,screen_width,screen_height;
 int32_t selected_tile,selection_active,x1,y1,x2,y2;
 int32_t clipboard_active,clipboard_width,clipboard_height;
 int32_t selection_color,clipboard_color,palette_background_color;
} SniperEditorRenderState;
enum { SNIPER_EDITOR_CURSOR=1,SNIPER_EDITOR_LOCK,SNIPER_EDITOR_HORIZONTAL,
 SNIPER_EDITOR_VERTICAL,SNIPER_EDITOR_UNLOCK,SNIPER_EDITOR_RECTANGLE,SNIPER_EDITOR_PALETTE };
/* Retained numeric command6 ABI; old name incorrectly implied a filled box. */
#define SNIPER_EDITOR_FILL_RECT SNIPER_EDITOR_RECTANGLE
/* CURSOR/PALETTE use x,y. Lines use x,y,length,color. Rectangle OUTLINE uses
 x,y,right,bottom,color, inclusive right/bottom edges (415f80). LOCK returns nonzero on success; other returns ignored.
 The adapter must bound unsafe native line spans before writing pixels. */
typedef struct { int32_t kind,x,y,length,right,bottom,color; } SniperEditorDrawCommand;
typedef int (*SniperEditorDraw)(void *,const SniperEditorDrawCommand *);
void sniper_editor_render(const SniperEditorRenderState *,SniperEditorDraw,void *);
#endif
