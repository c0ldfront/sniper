#ifndef SNIPER_TEXT_PANEL_H
#define SNIPER_TEXT_PANEL_H
#include <stdint.h>
typedef struct {const char *text;int32_t color;} SniperPanelRow;
typedef struct {int32_t width,height,columns,stride;} SniperTextPanelLayout;
typedef struct {
    void (*line)(void *,int vertical,int32_t x,int32_t y,int32_t length);
    void (*text)(void *,const char *,int32_t x,int32_t y,int32_t color);
    void *user;
} SniperTextPanelDraw;
/* Geometry and draw order from412661..41294b. Surface ownership/fill/lock,
 * palette and final blit belong to caller. Returns0 for empty/invalid input.
 * Original titleless panels still place rows at y15. Title width does not
 * enlarge the panel. Valid count1..256 and rows_per_column>0. */
int sniper_text_panel_layout(const char *title,const SniperPanelRow *,int32_t count,
    int32_t rows_per_column,SniperTextPanelLayout *);
void sniper_text_panel_draw(const char *title,const SniperPanelRow *,int32_t count,
    int32_t rows_per_column,const SniperTextPanelLayout *,const SniperTextPanelDraw *);
#endif
