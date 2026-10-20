#ifndef SNIPER87_IDLE_BITMAP_H
#define SNIPER87_IDLE_BITMAP_H
#include <stdint.h>
#include <stddef.h>
/* Original4124b0: bitcount<=8 gives2^bitcount RGB entries with flags0.
 * clrUsed is ignored; table starts at fixed offset40. Returns0 for short input. */
int sniper87_idle_palette_entries(const uint8_t *dib,size_t size,uint8_t entries[256][4],uint32_t *count);
#ifdef _WIN32
#include <windows.h>
HBITMAP sniper87_idle_bitmap_load(HINSTANCE,unsigned resource,HPALETTE *palette);
/* Caller owns BeginPaint/EndPaint; helper loads151,selects/realizes palettes,
 * centers bitmap,and releases bitmap/palette/DC. */
BOOL sniper87_idle_bitmap_paint(HWND,HDC,HINSTANCE);
#endif
#endif
