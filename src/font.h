#ifndef SNIPER_FONT_H
#define SNIPER_FONT_H
#include "sprite.h"
extern const SniperSprite sniper_fonts[4];
typedef void (*SniperGlyphDraw)(void *,int color,int32_t x,int32_t y,int32_t frame);
/* Original415d60; each byte is sign-extended before subtracting32. Blits
 * uncentered and transparent. Invalid/control bytes are not sanitized here. */
int32_t sniper_font_draw(const char *,int32_t x,int32_t y,int color,SniperGlyphDraw,void *);
int32_t sniper_font_width(const char *);
#endif
