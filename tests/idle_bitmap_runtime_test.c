#include "idle_bitmap.h"
#include <stdio.h>
#define CHECK(x) do {if(!(x)){printf("FAIL%d\n",__LINE__);return 1;}}while(0)
int main(void) {
    HINSTANCE instance=GetModuleHandle(NULL);HPALETTE pal;HBITMAP image=sniper87_idle_bitmap_load(instance,151,&pal);CHECK(image&&pal);
    HRSRC resource=FindResourceA(instance,MAKEINTRESOURCEA(151),RT_BITMAP);const unsigned char *dib=LockResource(LoadResource(instance,resource));
    PALETTEENTRY entries[256];CHECK(GetPaletteEntries(pal,0,256,entries)==256);
    for(int i=0;i<256;++i)CHECK(entries[i].peRed==dib[42+4*i]&&entries[i].peGreen==dib[41+4*i]&&entries[i].peBlue==dib[40+4*i]&&entries[i].peFlags==0);
    BITMAP b;CHECK(GetObject(image,sizeof b,&b));CHECK(b.bmWidth==320&&b.bmHeight==240);DeleteObject(image);DeleteObject(pal);
    HWND w=CreateWindowA("STATIC","idle-test",WS_POPUP,0,0,640,480,NULL,NULL,instance,NULL);CHECK(w);
    HDC dc=CreateCompatibleDC(NULL);CHECK(dc);BITMAPINFO info={0};info.bmiHeader.biSize=40;info.bmiHeader.biWidth=640;info.bmiHeader.biHeight=-480;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    void *bits;HBITMAP target=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,NULL,0);CHECK(target);HGDIOBJ old=SelectObject(dc,target);PatBlt(dc,0,0,640,480,BLACKNESS);
    CHECK(sniper87_idle_bitmap_paint(w,dc,instance));GdiFlush();unsigned *pixels=bits;
    for(int y=0;y<480;++y)for(int x=0;x<640;++x){unsigned expected=0;
        if(x>=160&&x<480&&y>=120&&y<360){unsigned i=dib[1064+(239-(y-120))*320+x-160];expected=entries[i].peBlue|((unsigned)entries[i].peGreen<<8)|((unsigned)entries[i].peRed<<16);}
        if((pixels[y*640+x]&0xffffff)!=expected){printf("FAILpixel%d,%d got%x expected%x\n",x,y,pixels[y*640+x],expected);return 2;}
    }
    SelectObject(dc,old);DeleteObject(target);DeleteDC(dc);DestroyWindow(w);
    pal=(HPALETTE)1;CHECK(!sniper87_idle_bitmap_load(instance,65534,&pal)&&!pal);
    puts("PASS256 palette entries and307200 rendered pixels: original151 bitmap,centered placement,background preservation,missing-resource cleanup");return 0;
}
