#ifndef SNIPER87_DISPLAY_H
#define SNIPER87_DISPLAY_H
#include <windows.h>
typedef struct { int width,height; unsigned supported; } SniperDisplay;
void sniper_display_load(HWND window,SniperDisplay *display);
void sniper_display_save(const SniperDisplay *display);
void sniper_display_menu(HMENU menu,const SniperDisplay *display);
int sniper_display_command(SniperDisplay *display,unsigned command);
#endif
