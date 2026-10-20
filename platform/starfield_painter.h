#ifndef SNIPER_STARFIELD_PAINTER_H
#define SNIPER_STARFIELD_PAINTER_H
#include "sprite_surface.h"
#include "../src/starfield.h"
/* Indexed branch of412300;color probe restores pixel0,0. */
HRESULT sniper_indexed_color(LPDIRECTDRAWSURFACE,COLORREF,uint8_t *);
/* Resolve original RGB(92,92,92) and RGB(48,48,48) through a palette-bearing
 * indexed surface (original uses the tile atlas). Restores probe pixel. */
HRESULT sniper_starfield_colors(LPDIRECTDRAWSURFACE,uint8_t *near_color,uint8_t *far_color);
/* 409840 surface lifecycle with recovered drawing. Requires indexed8 target.
 * Does not clear target; returns Lock/Unlock failures to caller. */
HRESULT sniper_starfield_paint(const SniperStarfield *,LPDIRECTDRAWSURFACE,
    int32_t width,int32_t height,int32_t camera_x,int32_t camera_y,
    uint8_t near_color,uint8_t far_color);
#endif
