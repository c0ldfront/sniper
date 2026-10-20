#ifndef SNIPER_GAME_DISPLAY_H
#define SNIPER_GAME_DISPLAY_H
#define COBJMACROS
#include <windows.h>
#include <ddraw.h>
typedef struct {
    LPDIRECTDRAW draw;
    LPDIRECTDRAWSURFACE primary,backbuffer;
    HWND window;
    int width,height,backbuffer_count;
} SniperGameDisplay;
/* Owns DirectDraw and surfaces. Original402a05..402cd9 exclusive8-bit chain.
 * Initialize state to zero. On failure releases owned resources and restores
 * desktop/cooperative mode. Returns HRESULT for caller's startup error UI. */
HRESULT sniper_game_display_open(SniperGameDisplay *,HWND,int width,int height);
void sniper_game_display_close(SniperGameDisplay *);
#endif
