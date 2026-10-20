#include "palette_resource.h"
#include "../src/palette.h"
HRESULT sniper_palette_create_resource(LPDIRECTDRAW dd,HINSTANCE instance,unsigned resource,LPDIRECTDRAWPALETTE *out){
    SniperPaletteEntry entries[256];const uint8_t *dib=NULL;size_t size=0;
    *out=NULL;
    if(resource){
        HRSRC found=FindResourceA(instance,MAKEINTRESOURCEA(resource),RT_BITMAP);
        if(!found)return E_FAIL;
        HGLOBAL loaded=LoadResource(instance,found);
        if(!loaded)return E_FAIL;
        dib=LockResource(loaded);size=SizeofResource(instance,found);
        if(!dib)return E_FAIL;
    }
    if(sniper_palette_resource(entries,dib,size))return E_INVALIDARG;
    return IDirectDraw_CreatePalette(dd,DDPCAPS_8BIT,(PALETTEENTRY *)entries,out,NULL);
}
