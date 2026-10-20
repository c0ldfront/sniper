#include "idle_bitmap.h"
#include <string.h>
int sniper87_idle_palette_entries(const uint8_t *dib,size_t size,uint8_t entries[256][4],uint32_t *count) {
    if(!dib || !entries || !count || size<40)return 0;
    uint32_t bits=(uint32_t)dib[14]|((uint32_t)dib[15]<<8);
    *count=bits<=8?1u<<bits:0;
    if(size<40+4u*(*count))return 0;
    for(uint32_t i=0;i<*count;++i){entries[i][0]=dib[42+4*i];entries[i][1]=dib[41+4*i];entries[i][2]=dib[40+4*i];entries[i][3]=0;}
    return 1;
}
#ifdef _WIN32
#include <stdlib.h>
HBITMAP sniper87_idle_bitmap_load(HINSTANCE instance,unsigned resource,HPALETTE *palette) {
    if(!palette)return NULL;
    *palette=NULL;HRSRC found=FindResourceA(instance,MAKEINTRESOURCEA(resource),RT_BITMAP);if(!found)return NULL;
    HGLOBAL loaded=LoadResource(instance,found);if(!loaded)return NULL;
    const uint8_t *dib=LockResource(loaded);DWORD size=SizeofResource(instance,found);uint8_t entries[256][4];uint32_t count;
    if(!sniper87_idle_palette_entries(dib,size,entries,&count))return NULL;
    uint32_t header=(uint32_t)dib[0]|((uint32_t)dib[1]<<8)|((uint32_t)dib[2]<<16)|((uint32_t)dib[3]<<24);
    if(header<40 || header>size || count*4u>size-header)return NULL;
    if(count){LOGPALETTE *p=malloc(sizeof(LOGPALETTE)+(count-1)*sizeof(PALETTEENTRY));if(!p)return NULL;
        p->palVersion=0x300;p->palNumEntries=(WORD)count;memcpy(p->palPalEntry,entries,count*4u);*palette=CreatePalette(p);free(p);}
    HDC dc=GetDC(NULL);if(!dc){if(*palette)DeleteObject(*palette);*palette=NULL;return NULL;}
    HPALETTE previous=NULL;if(*palette){previous=SelectPalette(dc,*palette,FALSE);RealizePalette(dc);}
    HBITMAP bitmap=CreateDIBitmap(dc,(const BITMAPINFOHEADER *)dib,CBM_INIT,dib+header+count*4u,(const BITMAPINFO *)dib,DIB_RGB_COLORS);
    if(previous)SelectPalette(dc,previous,FALSE);
    ReleaseDC(NULL,dc);
    if(!bitmap){if(*palette)DeleteObject(*palette);*palette=NULL;}
    return bitmap;
}
BOOL sniper87_idle_bitmap_paint(HWND window,HDC dc,HINSTANCE instance) {
    HPALETTE palette=NULL;HBITMAP bitmap=sniper87_idle_bitmap_load(instance,151,&palette);if(!bitmap)return FALSE;
    HDC source=CreateCompatibleDC(dc);BOOL result=FALSE;
    if(source){
        HPALETTE old_target=NULL,old_source=NULL;
        if(palette){old_target=SelectPalette(dc,palette,FALSE);RealizePalette(dc);old_source=SelectPalette(source,palette,FALSE);RealizePalette(source);}
        HGDIOBJ old=SelectObject(source,bitmap);BITMAP info;RECT client;
        if(GetObjectA(bitmap,sizeof info,&info) && GetClientRect(window,&client))
            result=BitBlt(dc,client.right/2-info.bmWidth/2,client.bottom/2-info.bmHeight/2,info.bmWidth,info.bmHeight,source,0,0,SRCCOPY);
        SelectObject(source,old);
        if(old_source)SelectPalette(source,old_source,FALSE);
        if(old_target)SelectPalette(dc,old_target,FALSE);
        DeleteDC(source);
    }
    DeleteObject(bitmap);if(palette)DeleteObject(palette);return result;
}
#endif
