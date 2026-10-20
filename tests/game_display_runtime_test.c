#include "game_display.h"
#include <stdio.h>
static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM a,LPARAM b){return DefWindowProcA(w,m,a,b);}
int main(void) {
    WNDCLASSA cls={0};cls.lpfnWndProc=proc;cls.hInstance=GetModuleHandle(NULL);cls.lpszClassName="display-test";
    RegisterClassA(&cls);HWND window=CreateWindowA(cls.lpszClassName,"display",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,640,480,NULL,NULL,cls.hInstance,NULL);
    LPDIRECTDRAW probe=NULL;DDSURFACEDESC before={0};before.dwSize=sizeof before;
    if(FAILED(DirectDrawCreate(NULL,&probe,NULL)))return 8;
    if(FAILED(IDirectDraw_GetDisplayMode(probe,&before)))return 9;
    IDirectDraw_Release(probe);
    SniperGameDisplay d={0};HRESULT hr=sniper_game_display_open(&d,window,640,480);
    if(FAILED(hr)){printf("FAIL open hr=%08lx\n",(unsigned long)hr);DestroyWindow(window);return 1;}
    DDSURFACEDESC mode={0};mode.dwSize=sizeof mode;hr=IDirectDraw_GetDisplayMode(d.draw,&mode);
    if(FAILED(hr)||mode.dwWidth!=640||mode.dwHeight!=480||mode.ddpfPixelFormat.dwRGBBitCount!=8)return 2;
    DDBLTFX fill={0};fill.dwSize=sizeof fill;fill.dwFillColor=17;
    hr=IDirectDrawSurface_Blt(d.backbuffer,NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&fill);
    if(FAILED(hr))return 3;
    DDSURFACEDESC locked={0};locked.dwSize=sizeof locked;
    hr=IDirectDrawSurface_Lock(d.backbuffer,NULL,&locked,DDLOCK_WAIT,NULL);if(FAILED(hr))return 4;
    int valid=locked.ddpfPixelFormat.dwRGBBitCount==8 && *(unsigned char *)locked.lpSurface==17;
    IDirectDrawSurface_Unlock(d.backbuffer,locked.lpSurface);if(!valid)return 5;
    hr=IDirectDrawSurface_Flip(d.primary,NULL,DDFLIP_WAIT);if(FAILED(hr)){printf("FAIL flip hr=%08lx\n",(unsigned long)hr);return 6;}
    printf("PASS exclusive640x480x8 chain:%d backbuffers,indexed fill,primary flip\n",d.backbuffer_count);
    sniper_game_display_close(&d);
    if(d.draw||d.primary||d.backbuffer)return 7;
    sniper_game_display_close(&d);
    if(sniper_game_display_open(&d,window,-1,480)!=E_INVALIDARG||d.draw)return 10;
    DDSURFACEDESC after={0};after.dwSize=sizeof after;
    if(FAILED(DirectDrawCreate(NULL,&probe,NULL)))return 11;
    hr=IDirectDraw_GetDisplayMode(probe,&after);IDirectDraw_Release(probe);
    if(FAILED(hr)||after.dwWidth!=before.dwWidth||after.dwHeight!=before.dwHeight||
       after.ddpfPixelFormat.dwRGBBitCount!=before.ddpfPixelFormat.dwRGBBitCount)return 12;
    DestroyWindow(window);printf("PASS cleanup,repeated close,invalid-size rejection and desktop mode restoration\n");return 0;
}
