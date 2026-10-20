#include "palette_resource.h"
#include "sprite_surface.h"
#include "../src/ship_assets.h"
#include "../src/palette.h"
#include <stdio.h>
#define CHECK(call) do{HRESULT h=(call);if(FAILED(h)){printf("FAIL line%d hr=%08lx\n",__LINE__,(unsigned long)h);return 1;}}while(0)
int main(void){
    HWND w=CreateWindowA("STATIC","palette-test",WS_POPUP,0,0,1024,768,NULL,NULL,GetModuleHandle(NULL),NULL);
    LPDIRECTDRAW dd;CHECK(DirectDrawCreate(NULL,&dd,NULL));
    CHECK(IDirectDraw_SetCooperativeLevel(dd,w,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_ALLOWREBOOT));
    CHECK(IDirectDraw_SetDisplayMode(dd,1024,768,8));
    DDSURFACEDESC d={0};d.dwSize=sizeof d;d.dwFlags=DDSD_CAPS;d.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;
    LPDIRECTDRAWSURFACE primary;CHECK(IDirectDraw_CreateSurface(dd,&d,&primary,NULL));
    LPDIRECTDRAWPALETTE palette;CHECK(sniper_palette_create_resource(dd,GetModuleHandle(NULL),107,&palette));
    CHECK(IDirectDrawSurface_SetPalette(primary,palette));
    PALETTEENTRY entries[256];CHECK(IDirectDrawPalette_GetEntries(palette,0,0,256,entries));
    printf("Palette endpoints RGB %u,%u,%u / %u,%u,%u\n",entries[0].peRed,entries[0].peGreen,entries[0].peBlue,entries[255].peRed,entries[255].peGreen,entries[255].peBlue);
    HRSRC r=FindResourceA(GetModuleHandle(NULL),MAKEINTRESOURCEA(107),RT_BITMAP);
    HGLOBAL loaded=LoadResource(GetModuleHandle(NULL),r);SniperPaletteEntry expected_palette[256];
    if(sniper_palette_resource(expected_palette,LockResource(loaded),SizeofResource(GetModuleHandle(NULL),r)))return 5;
    for(unsigned i=1;i<255;++i){
        if(entries[i].peRed!=expected_palette[i].red || entries[i].peGreen!=expected_palette[i].green || entries[i].peBlue!=expected_palette[i].blue){printf("FAIL palette entry%u\n",i);return 6;}
    }
    if(entries[0].peRed || entries[0].peGreen || entries[0].peBlue || entries[255].peRed!=255 || entries[255].peGreen!=255 || entries[255].peBlue!=255)return 7;
    printf("PASS254 resource palette colors and reserved black/white endpoints\n");
    SniperSpriteSurface ship;CHECK(sniper_surface_create(dd,GetModuleHandle(NULL),&sniper_ship_assets[0],&ship));
    d=(DDSURFACEDESC){0};d.dwSize=sizeof d;CHECK(IDirectDrawSurface_GetSurfaceDesc(ship.surface,&d));
    printf("Sprite surface bit depth %lu\n",(unsigned long)d.ddpfPixelFormat.dwRGBBitCount);
    if(d.ddpfPixelFormat.dwRGBBitCount!=8)return 2;
    DDCOLORKEY key;CHECK(IDirectDrawSurface_GetColorKey(ship.surface,DDCKEY_SRCBLT,&key));
    printf("Native black key %lu..%lu\n",(unsigned long)key.dwColorSpaceLowValue,(unsigned long)key.dwColorSpaceHighValue);
    DDBLTFX fill={0};fill.dwSize=sizeof fill;fill.dwFillColor=42;
    CHECK(IDirectDrawSurface_Blt(primary,NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&fill));
    CHECK(sniper_surface_draw(&ship,primary,64,64,0,1,0));
    DDSURFACEDESC src={0},dst={0};src.dwSize=sizeof src;dst.dwSize=sizeof dst;
    CHECK(IDirectDrawSurface_Lock(ship.surface,NULL,&src,DDLOCK_WAIT|DDLOCK_READONLY,NULL));
    CHECK(IDirectDrawSurface_Lock(primary,NULL,&dst,DDLOCK_WAIT|DDLOCK_READONLY,NULL));
    unsigned copied=0,transparent=0;
    for(int y=0;y<32;++y)for(int x=0;x<32;++x){
        unsigned char value=((unsigned char*)src.lpSurface)[y*src.lPitch+x];
        unsigned char got=((unsigned char*)dst.lpSurface)[(48+y)*dst.lPitch+48+x];
        unsigned expected=value==key.dwColorSpaceLowValue ? 42 : value;
        if(got!=expected){printf("FAIL indexed blit%d,%d expected%u got%u\n",x,y,expected,got);return 3;}
        if(value==key.dwColorSpaceLowValue)++transparent;else ++copied;
    }
    IDirectDrawSurface_Unlock(primary,NULL);IDirectDrawSurface_Unlock(ship.surface,NULL);
    if(!copied || !transparent)return 4;
    printf("PASS indexed sprite blit: %u copied and %u transparent pixels\n",copied,transparent);
    sniper_surface_destroy(&ship);IDirectDrawSurface_Release(primary);IDirectDrawPalette_Release(palette);
    IDirectDraw_RestoreDisplayMode(dd);IDirectDraw_SetCooperativeLevel(dd,w,DDSCL_NORMAL);IDirectDraw_Release(dd);DestroyWindow(w);return 0;
}
