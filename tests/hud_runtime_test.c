#include "hud_painter.h"
#include "hud_panel_painter.h"
#include "palette_resource.h"
#include "../src/font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(call) do{HRESULT h_=(call);if(FAILED(h_)){printf("FAIL line%d hr=%08lx\n",__LINE__,(unsigned long)h_);return 1;}}while(0)
static uint32_t now(void *ctx){++*(int *)ctx;return 20000;}
static uint32_t last(void *ctx){++*(int *)ctx;return 0;}
static int reference_text_mode(uint8_t *pixels,int pitch,int width,int height,SniperSpriteSurface *font,const char *s,int x,int y,int opaque)
{
    DDSURFACEDESC d={0};d.dwSize=sizeof d;CHECK(IDirectDrawSurface_Lock(font->surface,NULL,&d,DDLOCK_WAIT,NULL));
    DDCOLORKEY key;CHECK(IDirectDrawSurface_GetColorKey(font->surface,DDCKEY_SRCBLT,&key));
    for(;*s;++s,x+=6){int frame=(unsigned char)*s-32,sx=frame%48*6,sy=frame/48*8;
        for(int iy=0;iy<8;++iy)for(int ix=0;ix<6;++ix){
            uint8_t c=((uint8_t *)d.lpSurface)[(sy+iy)*d.lPitch+sx+ix];
            if((opaque||c!=key.dwColorSpaceLowValue)&&x+ix>=0&&x+ix<width&&y+iy>=0&&y+iy<height)pixels[(y+iy)*pitch+x+ix]=c;
        }
    }
    CHECK(IDirectDrawSurface_Unlock(font->surface,d.lpSurface));return 0;
}
static int reference_text(uint8_t *pixels,int pitch,int width,int height,SniperSpriteSurface *font,const char *s,int x,int y)
{return reference_text_mode(pixels,pitch,width,height,font,s,x,y,0);}
int main(void)
{
    LPDIRECTDRAW dd;CHECK(DirectDrawCreate(NULL,&dd,NULL));
    HWND window=CreateWindowA("STATIC","hud-test",WS_OVERLAPPEDWINDOW,0,0,640,480,NULL,NULL,GetModuleHandle(NULL),NULL);
    CHECK(IDirectDraw_SetCooperativeLevel(dd,window,DDSCL_NORMAL));
    LPDIRECTDRAWPALETTE palette;CHECK(sniper_palette_create_resource(dd,GetModuleHandle(NULL),107,&palette));
    DDSURFACEDESC desc={0};desc.dwSize=sizeof desc;desc.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    desc.ddpfPixelFormat.dwSize=sizeof desc.ddpfPixelFormat;desc.ddpfPixelFormat.dwFlags=DDPF_RGB|DDPF_PALETTEINDEXED8;desc.ddpfPixelFormat.dwRGBBitCount=8;
    SniperSpriteSurface fonts[4]={0};
    for(int i=0;i<4;++i){fonts[i].descriptor=sniper_fonts[i];fonts[i].instance=GetModuleHandle(NULL);desc.dwWidth=288;desc.dwHeight=16;
        CHECK(IDirectDraw_CreateSurface(dd,&desc,&fonts[i].surface,NULL));CHECK(IDirectDrawSurface_SetPalette(fonts[i].surface,palette));CHECK(sniper_surface_restore(&fonts[i]));}
    for(int scene=0;scene<4;++scene){
        int width=scene==1?320:640,height=200;
        desc.dwWidth=width;desc.dwHeight=height;LPDIRECTDRAWSURFACE target;CHECK(IDirectDraw_CreateSurface(dd,&desc,&target,NULL));CHECK(IDirectDrawSurface_SetPalette(target,palette));
        SniperShipObject ship={0};ship.word[0x5c/4]=scene==0?900000:scene==1?500000:250000;
        ship.word[0xc8/4]=1000000;ship.word[0x108/4]=120;ship.word[0x10c/4]=91;
        int clock_calls=0;
        SniperHudStatus h={.local=&ship,.width=width,.height=height,.show_energy_bar=1,.maximum_energy=1000000,
            .blink=scene==2?1:6,.safety_ticks=scene==0?372300:0,.player_count=2,.connected=scene==0,
            .energy_high=11,.energy_medium=12,.energy_low=13,.border=14,.capacity=15,
            .milliseconds=now,.last_receive_tick=last,.user=&clock_calls};
        DDSURFACEDESC d={0};d.dwSize=sizeof d;CHECK(IDirectDrawSurface_Lock(target,NULL,&d,DDLOCK_WAIT,NULL));
        size_t size=(size_t)d.lPitch*height;uint8_t *reference=malloc(size);if(!reference)return 2;
        int pitch=d.lPitch;memset(reference,197,size);memset(d.lpSurface,197,size);CHECK(IDirectDrawSurface_Unlock(target,d.lpSurface));
        CHECK(sniper_hud_status_paint(&h,fonts,target));
        if(h.user!=&clock_calls||h.milliseconds!=now||h.last_receive_tick!=last||h.text||h.rect||h.vertical_line||clock_calls!=(scene==0?2:0))return 3;
        if(h.blink!=(scene==2?2:scene==3?7:6))return 4;
        const char *energy=scene==0?"900":scene==1?"500":"250";
        if(reference_text(reference,pitch,width,height,&fonts[scene==0?3:2],energy,width-19,16))return 5;
        int span=width/3,left=(width-span)/2,right=left+ship.word[0x5c/4]*span/1000000;
        if(scene!=2)for(int y=1;y<5;++y)memset(reference+y*pitch+left,scene==0?11:13,(size_t)(right-left));
        for(int y=0;y<6;++y){reference[y*pitch+left-1]=14;reference[y*pitch+left+span]=15;}
        if(reference_text(reference,pitch,width,height,&fonts[2],"Shields: 1.2",width-73,32))return 5;
        if(reference_text(reference,pitch,width,height,&fonts[2],"Super!: 0.9",width-67,40))return 5;
        if(scene==0){
            if(reference_text(reference,pitch,width,height,&fonts[2],"Safety Time Left: 1:02:03",width/2,height/2-48))return 5;
            if(reference_text(reference,pitch,width,height,&fonts[2],"WARNING: No data coming from server",width/6,height*2/5))return 5;
        }
        CHECK(IDirectDrawSurface_Lock(target,NULL,&d,DDLOCK_WAIT,NULL));
        for(size_t i=0;i<size;++i)if(reference[i]!=((uint8_t *)d.lpSurface)[i]){printf("FAIL scene%d offset%lu expected%u got%u\n",scene,(unsigned long)i,reference[i],((uint8_t *)d.lpSurface)[i]);return 6;}
        CHECK(IDirectDrawSurface_Unlock(target,d.lpSurface));free(reference);IDirectDrawSurface_Release(target);
    }
    puts("PASS four indexed HUD scenes: fonts,energy fill/markers,blink,timers,safety,network warning;320 and640px paths;callback context preserved");
    SniperHudPanels panels;sniper_hud_panels_init(&panels,dd,fonts);
    desc.dwWidth=640;desc.dwHeight=200;LPDIRECTDRAWSURFACE target;CHECK(IDirectDraw_CreateSurface(dd,&desc,&target,NULL));CHECK(IDirectDrawSurface_SetPalette(target,palette));
    SniperShipObject ship={0};ship.word[0x68/4]=7;ship.word[0xa0/4]=12;ship.word[0x5c/4]=900000;
    ship.word[0xd0/4]=2;ship.word[0xd4/4]=1;ship.word[0xd8/4]=3;ship.word[0xdc/4]=4;ship.word[0x100/4]=1;
    for(int i=0;i<5;++i){ship.word[0xec/4+i]=1;ship.word[0x88/4+i]=i%2==0;}
    uint8_t team[48*16],equipment[60*64];
    for(int frame=0;frame<4;++frame){
        if(frame==2){++ship.word[0x68/4];--ship.word[0xa0/4];ship.word[0x88/4]=0;ship.word[0x8c/4]=1;}
        if(frame==3){++ship.word[0xa0/4];++ship.word[0xd0/4];}
        if(frame==0||frame==3){
            memset(team,231,sizeof team);memset(equipment,231,sizeof equipment);
            const char *team_label=frame==0?"7 Ghz":"8 Ghz";
            if(reference_text_mode(team,48,48,16,&fonts[1],team_label,18,0,1))return 7;
            if(reference_text_mode(team,48,48,16,&fonts[1],"12 Bty",12,8,1))return 7;
            if(reference_text_mode(equipment,60,60,64,&fonts[1],frame==0?"Gun-2":"Gun-3",0,0,1))return 7;
            if(reference_text_mode(equipment,60,60,64,&fonts[1],"PBomb-1:3",0,8,1))return 7;
            const char *names[]={"Stealth","Cloak","Ghost","X-Radar","MultiFire"};
            for(int i=0;i<5;++i){int y=16+i*8;
                if(ship.word[0x88/4+i])for(int yy=y;yy<y+8;++yy)memset(equipment+yy*60,123,strlen(names[i])*6);
                if(reference_text_mode(equipment,60,60,64,&fonts[0],names[i],0,y,0))return 7;
            }
            if(reference_text_mode(equipment,60,60,64,&fonts[0],"Repel:4",0,56,1))return 7;
        }
        DDSURFACEDESC d={0};d.dwSize=sizeof d;CHECK(IDirectDrawSurface_Lock(target,NULL,&d,DDLOCK_WAIT,NULL));
        size_t size=(size_t)d.lPitch*200;uint8_t *expected=malloc(size);if(!expected)return 8;
        int pitch=d.lPitch;memset(expected,197,size);memset(d.lpSurface,197,size);CHECK(IDirectDrawSurface_Unlock(target,d.lpSurface));
        SniperHudStatus status={.local=&ship,.width=640,.height=200};
        CHECK(sniper_hud_paint(&panels,target,&status,231,123));
        if(reference_text(expected,pitch,640,200,&fonts[3],"900",621,16))return 10;
        for(int y=0;y<16;++y)for(int x=0;x<48;++x)if(team[y*48+x]!=231)expected[y*pitch+592+x]=team[y*48+x];
        for(int y=0;y<64;++y)for(int x=0;x<60;++x)if(equipment[y*60+x]!=231)expected[(y+100)*pitch+x]=equipment[y*60+x];
        CHECK(IDirectDrawSurface_Lock(target,NULL,&d,DDLOCK_WAIT,NULL));
        for(size_t i=0;i<size;++i)if(expected[i]!=((uint8_t *)d.lpSurface)[i]){printf("FAIL panels frame%d offset%lu expected%u got%u\n",frame,(unsigned long)i,expected[i],((uint8_t *)d.lpSurface)[i]);return 9;}
        CHECK(IDirectDrawSurface_Unlock(target,d.lpSurface));free(expected);
    }
    sniper_hud_panels_destroy(&panels);IDirectDrawSurface_Release(target);
    puts("PASS four combined status/cached HUD frames: opaque numerics,transparent toggle highlights,keyed panels,cache reuse,same-sum stale cache,rebuild");
    for(int i=0;i<4;++i)sniper_surface_destroy(&fonts[i]);
    IDirectDrawPalette_Release(palette);IDirectDraw_Release(dd);DestroyWindow(window);return 0;
}
