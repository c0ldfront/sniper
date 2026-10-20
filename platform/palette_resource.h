#ifndef SNIPER_PALETTE_RESOURCE_H
#define SNIPER_PALETTE_RESOURCE_H
#define COBJMACROS
#include <windows.h>
#include <ddraw.h>
HRESULT sniper_palette_create_resource(LPDIRECTDRAW,HINSTANCE,unsigned resource,LPDIRECTDRAWPALETTE *);
#endif
