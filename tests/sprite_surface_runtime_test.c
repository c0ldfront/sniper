#include "sprite_surface.h"
#include "ship_painter.h"
#include "map_painter.h"
#include "viewport.h"
#include "overlay_painter.h"
#include "projectile_painter.h"
#include "actor_painter.h"
#include "../src/ship_assets.h"
#include "../src/font.h"
#include <stdio.h>
#define CHECK(call) do { HRESULT h=(call);if(FAILED(h)){printf("FAIL line%d hr=%08lx\n",__LINE__,(unsigned long)h);return 1;} }while(0)
int main(void){
    LPDIRECTDRAW dd;CHECK(DirectDrawCreate(NULL,&dd,NULL));
    HWND window=CreateWindowA("STATIC","sprite-test",WS_OVERLAPPEDWINDOW,0,0,640,480,NULL,NULL,GetModuleHandle(NULL),NULL);
    CHECK(IDirectDraw_SetCooperativeLevel(dd,window,DDSCL_NORMAL));
    DDSURFACEDESC d={0};d.dwSize=sizeof d;d.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
    d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;d.dwWidth=128;d.dwHeight=128;
    LPDIRECTDRAWSURFACE target;CHECK(IDirectDraw_CreateSurface(dd,&d,&target,NULL));
    CHECK(sniper_viewport_clip(dd,target,128,128));
    unsigned long pixels=0;unsigned cases=0;
    for(int i=0;i<21;++i){
        const SniperSprite *desc=i<9 ? &sniper_ship_assets[i] : i<11 ? &sniper_effect_assets[i-9] : i<15 ? &sniper_fonts[i-11] : i==15 ? &sniper_pickup_sprite : &sniper_projectile_assets[i-16];
        SniperSpriteSurface surface;CHECK(sniper_surface_create(dd,GetModuleHandle(NULL),desc,&surface));
        HBITMAP bitmap=LoadImageA(GetModuleHandle(NULL),MAKEINTRESOURCEA(desc->resource),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
        HDC source=CreateCompatibleDC(NULL);HGDIOBJ old=SelectObject(source,bitmap);
        int frames=i>=16 ? desc->rows*desc->columns : desc->frames ? desc->frames : desc->rows*desc->columns;
        for(int frame=0;frame<frames;++frame){
            for(int opaque=0;opaque<2;++opaque){
                HDC dc;CHECK(IDirectDrawSurface_GetDC(target,&dc));
                HBRUSH brush=CreateSolidBrush(RGB(17,53,91));RECT area={0,0,128,128};FillRect(dc,&area,brush);DeleteObject(brush);
                COLORREF background=GetPixel(dc,0,0);IDirectDrawSurface_ReleaseDC(target,dc);
                CHECK(sniper_surface_draw(&surface,target,64,64,frame,1,opaque));
                CHECK(IDirectDrawSurface_GetDC(target,&dc));
                int sx=(frame%desc->columns)*desc->width+desc->origin_x;
                int sy=(frame/desc->columns)*desc->height+desc->origin_y;
                for(int y=0;y<desc->height;++y)for(int x=0;x<desc->width;++x){
                    COLORREF expected=GetPixel(source,sx+x,sy+y);
                    if(!opaque && expected==RGB(0,0,0))expected=background;
                    COLORREF got=GetPixel(dc,64-desc->width/2+x,64-desc->height/2+y);
                    if(got!=expected){printf("FAIL asset%d frame%d opaque%d pixel%d,%d expected%lx got%lx\n",i,frame,opaque,x,y,(unsigned long)expected,(unsigned long)got);return 2;}
                    ++pixels;
                }
                IDirectDrawSurface_ReleaseDC(target,dc);++cases;
            }
        }
        SelectObject(source,old);DeleteDC(source);DeleteObject(bitmap);sniper_surface_destroy(&surface);
    }
    const SniperSprite *edge_desc=&sniper_ship_assets[2];
    SniperSpriteSurface edge;CHECK(sniper_surface_create(dd,GetModuleHandle(NULL),edge_desc,&edge));
    HBITMAP edge_bitmap=LoadImageA(GetModuleHandle(NULL),MAKEINTRESOURCEA(700),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
    HDC edge_dc=CreateCompatibleDC(NULL);HGDIOBJ edge_old=SelectObject(edge_dc,edge_bitmap);
    const int positions[][2]={{-8,40},{120,40},{40,-8},{40,120},{-8,-8},{120,120},{-40,40},{140,40}};
    for(unsigned k=0;k<sizeof positions/sizeof *positions;++k){
        HDC dc;CHECK(IDirectDrawSurface_GetDC(target,&dc));PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
        CHECK(sniper_surface_draw(&edge,target,positions[k][0],positions[k][1],12,0,1));
        CHECK(IDirectDrawSurface_GetDC(target,&dc));
        for(int y=0;y<128;++y)for(int x=0;x<128;++x){
            int sx=x-positions[k][0],sy=y-positions[k][1];COLORREF expected=RGB(0,0,0);
            if(sx>=0 && sy>=0 && sx<32 && sy<32)expected=GetPixel(edge_dc,64+sx,256+32+sy);
            if(GetPixel(dc,x,y)!=expected){printf("FAIL clip%u pixel%d,%d\n",k,x,y);return 5;}
        }
        IDirectDrawSurface_ReleaseDC(target,dc);
    }
    SelectObject(edge_dc,edge_old);DeleteDC(edge_dc);DeleteObject(edge_bitmap);sniper_surface_destroy(&edge);
    printf("PASS8 clipping positions including fully outside sprites\n");
    SniperShipPainter painter;CHECK(sniper_ship_painter_create(&painter,dd,GetModuleHandle(NULL)));
    for(int type=0;type<4;++type){
        SniperShipObject ship={0};ship.word[0x40/4]=type;ship.word[0x44/4]=7;
        ship.word[0x48/4]=64000;ship.word[0x4c/4]=64000;ship.word[0x58/4]=12000;
        ship.word[0x6c/4]=-1;ship.word[0x5c/4]=250000;ship.word[0xc8/4]=1000000;
        uint32_t blink=0;HDC dc;RECT area={0,0,128,128};
        CHECK(IDirectDrawSurface_GetDC(target,&dc));PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
        SniperShipObject *players[1024]={0},*ordered[]={&ship};players[7]=&ship;
        SniperShipDrawOrder order={ordered,&ship,1,7,0,0,0,NULL,NULL};
        SniperShipPaintOptions options={players,&blink,0,1,50,35,10};
        CHECK(sniper_ships_paint(&painter,target,&order,&options));
        COLORREF actual[128*128];CHECK(IDirectDrawSurface_GetDC(target,&dc));
        for(int y=0;y<128;++y)for(int x=0;x<128;++x)actual[y*128+x]=GetPixel(dc,x,y);
        FillRect(dc,&area,GetStockObject(BLACK_BRUSH));IDirectDrawSurface_ReleaseDC(target,dc);
        CHECK(sniper_surface_draw(&painter.body[type],target,64,64,12,1,0));
        const char *label="250";
        for(int k=0;k<3;++k)CHECK(sniper_surface_draw(&painter.fonts[2],target,76+6*k,76,label[k]-32,0,0));
        CHECK(IDirectDrawSurface_GetDC(target,&dc));
        for(int y=0;y<128;++y)for(int x=0;x<128;++x)if(actual[y*128+x]!=GetPixel(dc,x,y)){printf("FAIL painter type%d pixel%d,%d\n",type,x,y);return 3;}
        IDirectDrawSurface_ReleaseDC(target,dc);
    }
    SniperMapPainter map_painter;CHECK(sniper_map_painter_create(&map_painter,dd,GetModuleHandle(NULL)));
    static uint8_t map[1024*1024];map[10*1024+10]=1;map[12*1024+12]=216;
    HDC map_dc;CHECK(IDirectDrawSurface_GetDC(target,&map_dc));PatBlt(map_dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,map_dc);
    CHECK(sniper_map_paint(&map_painter,target,map,160,160,128,128,17));
    COLORREF map_pixels[128*128];CHECK(IDirectDrawSurface_GetDC(target,&map_dc));
    for(int y=0;y<128;++y)for(int x=0;x<128;++x)map_pixels[y*128+x]=GetPixel(map_dc,x,y);
    PatBlt(map_dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,map_dc);
    CHECK(sniper_surface_draw(&map_painter.ordinary,target,0,0,0,0,1));
    CHECK(sniper_surface_draw(&map_painter.special[0],target,32,32,3,0,0));
    CHECK(IDirectDrawSurface_GetDC(target,&map_dc));
    for(int y=0;y<128;++y)for(int x=0;x<128;++x)if(map_pixels[y*128+x]!=GetPixel(map_dc,x,y)){printf("FAIL map pixel%d,%d\n",x,y);return 4;}
    IDirectDrawSurface_ReleaseDC(target,map_dc);sniper_map_painter_destroy(&map_painter);
    printf("PASS map painter ordinary+animated tile frame matches explicit reference draws\n");
    SniperOverlayPainter overlay;CHECK(sniper_overlay_painter_create(&overlay,dd,GetModuleHandle(NULL)));
    for(int frame=0;frame<10;++frame)for(int color=0;color<4;++color){
        SniperPickupObject object={frame*12,5,4,100,1};SniperPickupObject *items[]={&object};
        SniperNotices notices={0};notices.ticks[0]=10;notices.ticks[2]=10;
        strcpy(notices.text[0],"AB");strcpy(notices.text[2],"C");
        notices.color[0]=color;notices.color[2]=(color+1)%4;
        HDC dc;CHECK(IDirectDrawSurface_GetDC(target,&dc));PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
        CHECK(sniper_pickups_paint(&overlay,target,items,1,16,16));
        CHECK(sniper_notices_paint(&overlay,target,&notices,120,100));
        COLORREF actual[128*128];CHECK(IDirectDrawSurface_GetDC(target,&dc));
        for(int y=0;y<128;++y)for(int x=0;x<128;++x)actual[y*128+x]=GetPixel(dc,x,y);
        PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
        CHECK(sniper_surface_draw(&overlay.pickup,target,64,48,frame,0,0));
        CHECK(sniper_surface_draw(&overlay.fonts[color],target,20,60,'A'-32,0,0));
        CHECK(sniper_surface_draw(&overlay.fonts[color],target,26,60,'B'-32,0,0));
        CHECK(sniper_surface_draw(&overlay.fonts[(color+1)%4],target,20,76,'C'-32,0,0));
        CHECK(IDirectDrawSurface_GetDC(target,&dc));
        for(int y=0;y<128;++y)for(int x=0;x<128;++x)if(actual[y*128+x]!=GetPixel(dc,x,y)){printf("FAIL overlay frame%d color%d pixel%d,%d\n",frame,color,x,y);return 6;}
        IDirectDrawSurface_ReleaseDC(target,dc);
    }

    printf("PASS40 pickup+notice scenes match explicit reference draws\n");
    SniperProjectilePainter weapons;CHECK(sniper_projectile_painter_create(&weapons,dd,GetModuleHandle(NULL)));
    for(int asset=0;asset<5;++asset){
        int frames=asset==0?24:asset==1?30:asset==2?7:asset==3?44:10;
        for(int frame=0;frame<frames;++frame){
            SniperProjectile object={0};SniperProjectile *items[]={&object};
            object.word[1]=80000;object.word[2]=80000;
            if(asset==0){object.word[0x34/4]=frame<12?1:2;object.word[0x38/4]=(frame%12)/4;object.word[0x14/4]=(frame%4)*6;}
            else if(asset==1){object.word[0x34/4]=3;object.word[0x38/4]=frame/10;object.word[0x14/4]=(frame%10)*10;}
            else {object.word[0x34/4]=asset==2?1:asset==3?3:5;object.word[0x18/4]=asset==2?49-frame*7:asset==3?132-frame*3:50-frame*5;}
            HDC dc;CHECK(IDirectDrawSurface_GetDC(target,&dc));PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
            CHECK(sniper_projectiles_paint(&weapons,target,items,1,16,16,0));
            COLORREF actual[128*128];CHECK(IDirectDrawSurface_GetDC(target,&dc));
            for(int y=0;y<128;++y)for(int x=0;x<128;++x)actual[y*128+x]=GetPixel(dc,x,y);
            PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
            CHECK(sniper_surface_draw(&weapons.assets[asset],target,64,64,frame,1,0));
            CHECK(IDirectDrawSurface_GetDC(target,&dc));
            for(int y=0;y<128;++y)for(int x=0;x<128;++x)if(actual[y*128+x]!=GetPixel(dc,x,y)){printf("FAIL projectile asset%d frame%d pixel%d,%d\n",asset,frame,x,y);return 7;}
            IDirectDrawSurface_ReleaseDC(target,dc);
        }
    }
    for(int mode=0;mode<4;++mode){
        SniperShipObject ship={0};ship.word[0x40/4]=mode;ship.word[0x44/4]=7;
        ship.word[0x48/4]=64000;ship.word[0x4c/4]=64000;ship.word[0x58/4]=12000;
        ship.word[0x6c/4]=-1;ship.word[0x5c/4]=250000;ship.word[0xc8/4]=1000000;
        SniperShipObject *players[1024]={0},*ordered[]={&ship};players[7]=&ship;
        uint32_t blink=0;SniperShipDrawOrder order={ordered,&ship,1,7,0,0,0,NULL,NULL};
        SniperShipPaintOptions options={players,&blink,0,1,50,35,10};
        SniperPickupObject pickup={24,4,4,100,1};SniperPickupObject *picks[]={&pickup};
        SniperProjectile bullets[2]={0};SniperProjectile *shots[]={&bullets[0],&bullets[1]};
        for(int i=0;i<2;++i){bullets[i].word[1]=64000;bullets[i].word[2]=64000;bullets[i].word[0x34/4]=1;}
        bullets[1].word[0x18/4]=35;
        int32_t pc=1,qc=2,cx=0,cy=0;
        SniperActorLayers layers={picks,&pc,shots,&qc,&cx,&cy,NULL,NULL,NULL,NULL};
        HDC dc;CHECK(IDirectDrawSurface_GetDC(target,&dc));PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
        CHECK(sniper_actors_paint(&layers,&order,&options,&overlay,&weapons,&painter,target,mode%2));
        COLORREF actual[128*128];CHECK(IDirectDrawSurface_GetDC(target,&dc));
        for(int y=0;y<128;++y)for(int x=0;x<128;++x)actual[y*128+x]=GetPixel(dc,x,y);
        PatBlt(dc,0,0,128,128,BLACKNESS);IDirectDrawSurface_ReleaseDC(target,dc);
        CHECK(sniper_surface_draw(&overlay.pickup,target,64,64,2,0,0));
        CHECK(sniper_surface_draw(&weapons.assets[0],target,64,64,0,1,0));
        blink=0;CHECK(sniper_ship_paint(&painter,target,&ship,&ship,NULL,&blink,0,1,50,35,10,0,0));
        if(!(mode%2))CHECK(sniper_surface_draw(&weapons.assets[2],target,64,64,2,1,0));
        CHECK(IDirectDrawSurface_GetDC(target,&dc));
        for(int y=0;y<128;++y)for(int x=0;x<128;++x)if(actual[y*128+x]!=GetPixel(dc,x,y)){printf("FAIL actor layers mode%d pixel%d,%d\n",mode,x,y);return 8;}
        IDirectDrawSurface_ReleaseDC(target,dc);
    }
    printf("PASS4 overlapping pickup/bullet/ship/impact scenes match explicit layer reference\n");
    sniper_overlay_painter_destroy(&overlay);
    sniper_projectile_painter_destroy(&weapons);
    printf("PASS115 projectile scenes match explicit reference draws\n");
    sniper_ship_painter_destroy(&painter);
    printf("PASS4 integrated ship+energy-label frames match explicit sprite/glyph reference draws\n");
    IDirectDrawSurface_Release(target);IDirectDraw_Release(dd);DestroyWindow(window);
    printf("PASS %u frame/mode cases, %lu pixels across21 original sprite/font atlases\n",cases,pixels);return 0;
}
