#ifndef SNIPER_CHAT_PAINTER_H
#define SNIPER_CHAT_PAINTER_H
#include "sprite_surface.h"
#include "../src/chat_render.h"
typedef struct {
    LPDIRECTDRAWSURFACE surface;
    uint32_t last_build_tick;
} SniperChatPainter;
typedef struct {
    LPDIRECTDRAW draw;LPDIRECTDRAWSURFACE target;
    SniperSpriteSurface *fonts; /* Four indexed font sprites. */
    const char (*lines)[128];const int32_t *colors;const char *input;
    int32_t width,height,rows;
    int32_t *dirty,*enabled;
    int32_t ticker_enabled,ticker_interval;
    uint8_t background;
    uint32_t (*milliseconds)(void *);
    /* Must insert next tip and advance index, marking dirty as appropriate. */
    void (*ticker)(void *);
    int (*recover)(void *);void *user;
} SniperChatPaint;
void sniper_chat_close(SniperChatPainter *);
HRESULT sniper_chat_paint(SniperChatPainter *,SniperChatPaint *);
#endif
