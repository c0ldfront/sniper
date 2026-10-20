#include "game_graphics.h"
#include <stdio.h>
static int check_sprite(SniperSpriteSurface *s) {
    DDSURFACEDESC d={0};d.dwSize=sizeof d;
    if(FAILED(IDirectDrawSurface_Lock(s->surface,NULL,&d,DDLOCK_WAIT,NULL)))return 0;
    int okay=d.ddpfPixelFormat.dwRGBBitCount==8 && d.dwWidth==(DWORD)(s->descriptor.width*s->descriptor.columns)
        && d.dwHeight==(DWORD)(s->descriptor.height*s->descriptor.rows);
    unsigned nonzero=0;
    for(DWORD y=0;y<d.dwHeight;++y)for(DWORD x=0;x<d.dwWidth;++x)
        nonzero+=((unsigned char *)d.lpSurface)[y*d.lPitch+x]!=0;
    IDirectDrawSurface_Unlock(s->surface,d.lpSurface);
    return okay && nonzero;
}
int main(void) {
    HINSTANCE inst=GetModuleHandle(NULL);
    HWND window=CreateWindowA("STATIC","graphics",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,640,480,NULL,NULL,inst,NULL);
    SniperGameDisplay display={0};HRESULT hr=sniper_game_display_open(&display,window,640,480);
    if(FAILED(hr)){printf("FAIL display:%08lx\n",(unsigned long)hr);return 1;}
    SniperGameGraphics g={0};
    for(int pass=0;pass<2;++pass) {
        hr=sniper_game_graphics_open(&g,&display,inst);
        if(FAILED(hr)){printf("FAIL graphics:%08lx\n",(unsigned long)hr);return 2;}
        SniperSpriteSurface *groups[]={&g.map.ordinary,g.map.special,g.ships.body,g.ships.effects,g.ships.fonts,g.projectiles.assets,&g.overlay.pickup,g.overlay.fonts};
        const int counts[]={1,4,9,2,4,5,1,4};int total=0;
        for(int group=0;group<8;++group)for(int i=0;i<counts[group];++i) {
            if(!check_sprite(groups[group]+i)){printf("FAIL sprite%d/%d\n",group,i);return 3;}++total;
        }
        SniperShipObject local={0};local.word[0x5c/4]=900000;local.word[0xc8/4]=1000000;
        hr=sniper_hud_panels_paint(&g.panels,display.backbuffer,&local,640,480,g.colors[0],g.colors[9]);
        if(FAILED(hr)||!g.panels.team.surface||!g.panels.equipment.cache.surface)return 4;
        hr=sniper_surface_draw(&g.ships.body[0],display.backbuffer,320,240,3,1,0);if(FAILED(hr))return 5;
        hr=IDirectDrawSurface_Flip(display.primary,NULL,DDFLIP_WAIT);if(FAILED(hr))return 6;
        sniper_game_graphics_close(&g);
        if(g.palette||g.panels.team.surface||g.ships.body[0].surface)return 7;
        sniper_game_graphics_close(&g);
        printf("PASS pass%d:%d indexed resource surfaces,HUD cache creation,sprite draw,primary flip,cleanup\n",pass,total);
    }
    sniper_game_display_close(&display);DestroyWindow(window);return 0;
}
