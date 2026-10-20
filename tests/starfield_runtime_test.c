#include "starfield_painter.h"
#include "fill_painter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(call) do{HRESULT hr_=(call);if(FAILED(hr_)){printf("FAIL line%d hr=%08lx\n",__LINE__,(unsigned long)hr_);return 1;}}while(0)
int main(void)
{
    LPDIRECTDRAW dd;CHECK(DirectDrawCreate(NULL,&dd,NULL));
    HWND window=CreateWindowA("STATIC","starfield-test",WS_OVERLAPPEDWINDOW,0,0,640,480,NULL,NULL,GetModuleHandle(NULL),NULL);
    CHECK(IDirectDraw_SetCooperativeLevel(dd,window,DDSCL_NORMAL));
    DDSURFACEDESC d={0};d.dwSize=sizeof d;
    d.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    d.dwWidth=641;d.dwHeight=481;d.ddpfPixelFormat.dwSize=sizeof d.ddpfPixelFormat;
    d.ddpfPixelFormat.dwFlags=DDPF_RGB|DDPF_PALETTEINDEXED8;d.ddpfPixelFormat.dwRGBBitCount=8;
    LPDIRECTDRAWSURFACE target;CHECK(IDirectDraw_CreateSurface(dd,&d,&target,NULL));
    PALETTEENTRY entries[256];uint8_t expected_near=0,expected_far=0;
    for(int i=0;i<256;++i){uint8_t gray=(uint8_t)(i*73);entries[i]=(PALETTEENTRY){gray,gray,gray,0};if(gray==92)expected_near=(uint8_t)i;if(gray==48)expected_far=(uint8_t)i;}
    LPDIRECTDRAWPALETTE palette;CHECK(IDirectDraw_CreatePalette(dd,DDPCAPS_8BIT|DDPCAPS_ALLOW256,entries,&palette,NULL));
    CHECK(IDirectDrawSurface_SetPalette(target,palette));
    DDSURFACEDESC lock={0};lock.dwSize=sizeof lock;
    CHECK(IDirectDrawSurface_Lock(target,NULL,&lock,DDLOCK_WAIT,NULL));
    memset(lock.lpSurface,197,(size_t)lock.lPitch*481);
    CHECK(IDirectDrawSurface_Unlock(target,lock.lpSurface));
    uint8_t near_color,far_color;CHECK(sniper_starfield_colors(target,&near_color,&far_color));
    if(near_color!=expected_near||far_color!=expected_far){printf("FAIL palette %u/%u expected %u/%u\n",near_color,far_color,expected_near,expected_far);return 2;}
    CHECK(IDirectDrawSurface_Lock(target,NULL,&lock,DDLOCK_WAIT,NULL));
    for(int i=0;i<lock.lPitch*481;++i)if(((uint8_t *)lock.lpSurface)[i]!=197){puts("FAIL color probe changed surface");return 3;}
    CHECK(IDirectDrawSurface_Unlock(target,lock.lpSurface));
    static SniperStarfield field;uint32_t seed=87;sniper_starfield_generate(&field,&seed);
    if(sniper_starfield_paint(&field,target,642,481,0,0,near_color,far_color)!=E_INVALIDARG)return 8;
    const int camera[][2]={{-100,-100},{0,0},{319,320},{8100,8200},{15000,15000}};
    unsigned long painted=0;
    for(unsigned c=0;c<sizeof camera/sizeof *camera;++c){
        CHECK(IDirectDrawSurface_Lock(target,NULL,&lock,DDLOCK_WAIT,NULL));
        size_t size=(size_t)lock.lPitch*481;uint8_t *reference=malloc(size);if(!reference)return 4;
        memset(reference,197,size);memset(lock.lpSurface,197,size);
        if(sniper_starfield_draw(&field,reference,lock.lPitch,641,481,camera[c][0],camera[c][1],near_color,far_color))return 5;
        CHECK(IDirectDrawSurface_Unlock(target,lock.lpSurface));
        CHECK(sniper_starfield_paint(&field,target,641,481,camera[c][0],camera[c][1],near_color,far_color));
        CHECK(IDirectDrawSurface_Lock(target,NULL,&lock,DDLOCK_WAIT,NULL));
        if(memcmp(reference,lock.lpSurface,size)){printf("FAIL scene%u\n",c);return 6;}
        for(size_t i=0;i<size;++i)painted+=reference[i]!=197;
        CHECK(IDirectDrawSurface_Unlock(target,lock.lpSurface));free(reference);
    }
    if(!painted)return 7;
    printf("PASS five indexed DirectDraw starfield scenes, %lu painted pixels; palette indices %u/%u; color probe restores surface\n",painted,near_color,far_color);
    SniperFillSurface fill;sniper_fill_surface_bind(&fill,target);
    for(int mode=0;mode<3;++mode){
        CHECK(IDirectDrawSurface_Lock(target,NULL,&lock,DDLOCK_WAIT,NULL));
        size_t size=(size_t)lock.lPitch*481;uint8_t *reference=malloc(size);if(!reference)return 9;
        memset(reference,197,size);memset(lock.lpSurface,197,size);
        int pitch=lock.lPitch;CHECK(IDirectDrawSurface_Unlock(target,lock.lpSurface));
        int back=mode!=2,width=mode==1?641:320;
        SniperFillRect r={11,7,28,13};
        CHECK((HRESULT)sniper_surface_fill(&fill,back,width,&r,124));
        CHECK((HRESULT)sniper_surface_vertical(&fill,back,width,35,2,19,176));
        CHECK((HRESULT)sniper_surface_horizontal(&fill,back,width,5,30,51,92));
        for(int y=7;y<13;++y)memset(reference+y*pitch+11,124,17);
        for(int y=2;y<21;++y)reference[y*pitch+35]=176;
        memset(reference+30*pitch+5,92,51);
        CHECK(IDirectDrawSurface_Lock(target,NULL,&lock,DDLOCK_WAIT,NULL));
        if(memcmp(reference,lock.lpSurface,size)){printf("FAIL fill mode%d\n",mode);return 10;}
        CHECK(IDirectDrawSurface_Unlock(target,lock.lpSurface));free(reference);
    }
    puts("PASS indexed rectangle/vertical/horizontal fills: backbuffer320 Blt, backbuffer641 memory, other-surface320 memory");
    IDirectDrawSurface_Release(target);IDirectDrawPalette_Release(palette);IDirectDraw_Release(dd);DestroyWindow(window);
    return 0;
}
