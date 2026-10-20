#include "sprite_surface.h"
#include <stdio.h>
#include <string.h>
#define REQUIRE(x) do {if(!(x)){printf("FAIL line%d\n",__LINE__);return 1;}}while(0)
/* Run only in an isolated fixture directory: the test owns ships.bmp. */
static int fixture(int topdown) {
    FILE *f=fopen("ships.bmp","wb");if(!f)return 0;
    BITMAPFILEHEADER fh={0};BITMAPINFOHEADER ih={0};RGBQUAD pal[256]={0};
    fh.bfType=0x4d42;fh.bfOffBits=sizeof fh+sizeof ih+sizeof pal;fh.bfSize=fh.bfOffBits+8;
    ih.biSize=sizeof ih;ih.biWidth=3;ih.biHeight=topdown?-2:2;ih.biPlanes=1;ih.biBitCount=8;ih.biSizeImage=8;ih.biClrUsed=256;
    pal[1]=(RGBQUAD){30,20,10,0};pal[2]=(RGBQUAD){60,50,40,0};
    unsigned char pixels[8]={1,2,1,0,2,1,2,0};
    fwrite(&fh,sizeof fh,1,f);fwrite(&ih,sizeof ih,1,f);fwrite(pal,sizeof pal,1,f);fwrite(pixels,8,1,f);return fclose(f)==0;
}
int main(void) {
    HINSTANCE instance=GetModuleHandle(NULL);HBITMAP bitmap;BITMAP bm;
    REQUIRE(GetFileAttributesA("ships.bmp")==INVALID_FILE_ATTRIBUTES);
    bitmap=sniper_surface_load_bitmap(instance,700);REQUIRE(bitmap);REQUIRE(GetObject(bitmap,sizeof bm,&bm));REQUIRE(bm.bmWidth==320 && bm.bmHeight==640);DeleteObject(bitmap);
    for(int top=0;top<1;++top) {
        REQUIRE(fixture(top));bitmap=sniper_surface_load_bitmap(instance,700);REQUIRE(bitmap);
        REQUIRE(GetObject(bitmap,sizeof bm,&bm));REQUIRE(bm.bmWidth==3 && bm.bmHeight==2 && bm.bmBitsPixel==8 && bm.bmBits);
        HDC dc=CreateCompatibleDC(NULL);HGDIOBJ old=SelectObject(dc,bitmap);
        REQUIRE(GetPixel(dc,0,0)==(top?RGB(10,20,30):RGB(40,50,60)));
        REQUIRE(GetPixel(dc,1,1)==(top?RGB(10,20,30):RGB(40,50,60)));
        SelectObject(dc,old);DeleteDC(dc);DeleteObject(bitmap);
        /* A resource-only sprite ignores arbitrary external files. */
        REQUIRE(sniper_surface_override_name(154)==NULL);
    }
    FILE *f=fopen("ships.bmp","wb");REQUIRE(f);fputs("invalid",f);fclose(f);
    REQUIRE(sniper_surface_load_bitmap(instance,700)==NULL);
    REQUIRE(DeleteFileA("ships.bmp"));bitmap=sniper_surface_load_bitmap(instance,700);REQUIRE(bitmap);DeleteObject(bitmap);
    printf("PASS resource fallback, indexed bottom-up override, padded rows, palette colors, invalid-file no-fallback, removal reload\n");return 0;
}
