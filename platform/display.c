/* Original mode enumeration 40d070/40d450, command/menu branches40ae10. */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "display.h"
#include <ddraw.h>
#include <stdio.h>
static const struct { unsigned command; int width,height; } modes[7]={
    {40004,320,200},{40020,320,240},{40005,640,480},{40006,800,600},
    {40007,1024,768},{40018,1280,1024},{40019,1600,1200}
};
static HRESULT CALLBACK observe_mode(LPDDSURFACEDESC mode,LPVOID context) {
    SniperDisplay *d=context;
    if (mode->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
        for (unsigned i=0;i<7;++i)
            if (mode->dwWidth==(DWORD)modes[i].width && mode->dwHeight==(DWORD)modes[i].height)
                d->supported|=1u<<i;
    return DDENUMRET_OK;
}
void sniper_display_load(HWND window,SniperDisplay *d) {
    d->width=(int)GetPrivateProfileIntA("Misc","ResolutionX",640,"sniper.ini");
    d->height=(int)GetPrivateProfileIntA("Misc","ResolutionY",480,"sniper.ini");
    /* Original data globals42225c..422274 initially include640x480. */
    d->supported=1u<<2;
    LPDIRECTDRAW dd=NULL;
    if (DirectDrawCreate(NULL,&dd,NULL)==DD_OK) {
        IDirectDraw_SetCooperativeLevel(dd,window,0x55);
        IDirectDraw_EnumDisplayModes(dd,0,NULL,d,observe_mode);
        IDirectDraw_Release(dd);
    }
}
void sniper_display_save(const SniperDisplay *d) {
    char number[32];snprintf(number,sizeof number,"%d",d->width);
    WritePrivateProfileStringA("Misc","ResolutionX",number,"sniper.ini");
    snprintf(number,sizeof number,"%d",d->height);
    WritePrivateProfileStringA("Misc","ResolutionY",number,"sniper.ini");
}
void sniper_display_menu(HMENU menu,const SniperDisplay *d) {
    for (unsigned i=0;i<7;++i) {
        int checked=d->width==modes[i].width && (i>1 || d->height==modes[i].height);
        CheckMenuItem(menu,modes[i].command,MF_BYCOMMAND|(checked?MF_CHECKED:MF_UNCHECKED));
        EnableMenuItem(menu,modes[i].command,MF_BYCOMMAND|((d->supported&(1u<<i))?MF_ENABLED:MF_GRAYED));
    }
}
int sniper_display_command(SniperDisplay *d,unsigned command) {
    for (unsigned i=0;i<7;++i) if(command==modes[i].command) {
        d->width=modes[i].width;d->height=modes[i].height;return 1;
    }
    return 0;
}
