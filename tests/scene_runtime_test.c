#include "scene_painter.h"
#include "display_recovery_painter.h"
#include "palette_resource.h"
#include "viewport.h"
#include "../src/font.h"
#include "../src/ship_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(call) do{HRESULT h_=(call);if(FAILED(h_)){printf("FAIL line%d hr=%08lx\n",__LINE__,(unsigned long)h_);return 1;}}while(0)
static HRESULT indexed_sprite(LPDIRECTDRAW dd,LPDIRECTDRAWPALETTE palette,const SniperSprite *desc,SniperSpriteSurface *out)
{
    memset(out,0,sizeof *out);out->descriptor=*desc;out->instance=GetModuleHandle(NULL);
    DDSURFACEDESC d={0};d.dwSize=sizeof d;d.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
    d.dwWidth=desc->width*desc->columns;d.dwHeight=desc->height*desc->rows;d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    d.ddpfPixelFormat.dwSize=sizeof d.ddpfPixelFormat;d.ddpfPixelFormat.dwFlags=DDPF_RGB|DDPF_PALETTEINDEXED8;d.ddpfPixelFormat.dwRGBBitCount=8;
    HRESULT hr=IDirectDraw_CreateSurface(dd,&d,&out->surface,NULL);if(FAILED(hr))return hr;
    hr=IDirectDrawSurface_SetPalette(out->surface,palette);return FAILED(hr)?hr:sniper_surface_restore(out);
}
static HRESULT lost_stage(void *user,SniperSceneStage stage){(void)user;(void)stage;return DDERR_SURFACELOST;}
static uint32_t present(void *user){++*(int *)user;return 0;}
typedef struct {SniperScenePainter *scene;LPDIRECTDRAWPALETTE palette;SniperDisplayRecovery state;int calls,recoveries;} RecoveryTest;
static uint32_t loss_present(void *user){RecoveryTest *r=user;return ++r->calls==1?(uint32_t)DDERR_SURFACELOST:0;}
static int recover_scene(void *user){RecoveryTest *r=user;++r->recoveries;return sniper_scene_restore(r->scene,r->palette,&r->state);}
static int clear_surface(LPDIRECTDRAWSURFACE s)
{
    DDSURFACEDESC d={0};d.dwSize=sizeof d;CHECK(IDirectDrawSurface_Lock(s,NULL,&d,DDLOCK_WAIT,NULL));
    memset(d.lpSurface,0,(size_t)d.lPitch*d.dwHeight);CHECK(IDirectDrawSurface_Unlock(s,d.lpSurface));return 0;
}
int main(void)
{
    LPDIRECTDRAW dd;CHECK(DirectDrawCreate(NULL,&dd,NULL));HWND window=CreateWindowA("STATIC","scene",WS_OVERLAPPEDWINDOW,0,0,640,480,NULL,NULL,GetModuleHandle(NULL),NULL);
    CHECK(IDirectDraw_SetCooperativeLevel(dd,window,DDSCL_NORMAL));LPDIRECTDRAWPALETTE palette;CHECK(sniper_palette_create_resource(dd,GetModuleHandle(NULL),107,&palette));
    SniperSprite target_desc={0,0,0,640,200,1,1,0,0};
    DDSURFACEDESC d={0};d.dwSize=sizeof d;d.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;d.dwWidth=target_desc.width;d.dwHeight=target_desc.height;d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    d.ddpfPixelFormat.dwSize=sizeof d.ddpfPixelFormat;d.ddpfPixelFormat.dwFlags=DDPF_RGB|DDPF_PALETTEINDEXED8;d.ddpfPixelFormat.dwRGBBitCount=8;
    LPDIRECTDRAWSURFACE target,reference;CHECK(IDirectDraw_CreateSurface(dd,&d,&target,NULL));CHECK(IDirectDraw_CreateSurface(dd,&d,&reference,NULL));
    CHECK(IDirectDrawSurface_SetPalette(target,palette));CHECK(IDirectDrawSurface_SetPalette(reference,palette));CHECK(sniper_viewport_clip(dd,target,640,200));CHECK(sniper_viewport_clip(dd,reference,640,200));
    SniperOverlayPainter overlay={0};SniperShipPainter ships={0};SniperMapPainter map_painter={0};SniperProjectilePainter projectiles={0};
    for(int i=0;i<4;++i){CHECK(indexed_sprite(dd,palette,&sniper_fonts[i],&overlay.fonts[i]));ships.fonts[i]=overlay.fonts[i];}
    CHECK(indexed_sprite(dd,palette,&sniper_ship_assets[0],&ships.body[0]));CHECK(indexed_sprite(dd,palette,&sniper_pickup_sprite,&overlay.pickup));
    CHECK(indexed_sprite(dd,palette,&sniper_projectile_assets[0],&projectiles.assets[0]));CHECK(indexed_sprite(dd,palette,&sniper_projectile_assets[2],&projectiles.assets[2]));
    const SniperSprite tiles={107,0,0,16,16,8,19,0,0};CHECK(indexed_sprite(dd,palette,&tiles,&map_painter.ordinary));
    static uint8_t map[1024*1024];map[6*1024+4]=1;
    SniperShipObject ship={0};ship.word[0x44/4]=7;ship.word[0x48/4]=104000;ship.word[0x4c/4]=72000;ship.word[0x58/4]=12000;ship.word[0x6c/4]=-1;ship.word[0x5c/4]=900000;ship.word[0xc8/4]=1000000;ship.word[0x68/4]=7;
    SniperShipObject *ordered[]={&ship},*players[1024]={0};players[7]=&ship;uint32_t blink=0;
    SniperShipDrawOrder order={.ships=ordered,.local=&ship,.count=1,.local_id=7};SniperShipPaintOptions options={.players=players,.blink=&blink,.label_mode=1,.upper=50,.lower=35,.bonus=10};
    SniperPickupObject pickup={0};pickup.x=6;pickup.y=4;pickup.age=24;SniperPickupObject *pickups[]={&pickup};int32_t pickup_count=1,projectile_count=2,cx=0,cy=0;
    SniperProjectile bullet={0},impact={0};bullet.word[4/4]=104000;bullet.word[8/4]=72000;bullet.word[0x34/4]=1;
    impact=bullet;impact.word[0x18/4]=35;SniperProjectile *shots[]={&bullet,&impact};
    SniperActorLayers actors={.pickups=pickups,.pickup_count=&pickup_count,.projectiles=shots,.projectile_count=&projectile_count,.camera_x=&cx,.camera_y=&cy};
    SniperNotices notices={0};sniper_notice_push(&notices,"Scene test",100,2);
    SniperHudStatus hud={.local=&ship,.width=640,.height=200,.show_energy_bar=1,.maximum_energy=1000000,.energy_high=11,.border=14,.capacity=15};
    SniperHudPanels panels;sniper_hud_panels_init(&panels,dd,overlay.fonts);
    static SniperStarfield stars;uint32_t rng=87;sniper_starfield_generate(&stars,&rng);uint8_t near_color,far_color;CHECK(sniper_starfield_colors(map_painter.ordinary.surface,&near_color,&far_color));
    SniperSceneFrame frame={.stars=1,.local=&ship};int presentations=0;
    SniperScenePainter scene={.frame=&frame,.target=target,.width=640,.height=200,.camera_x=&cx,.camera_y=&cy,.map=map,.stars=&stars,.near_color=near_color,.far_color=far_color,.map_painter=&map_painter,.actors=&actors,.ship_order=&order,.ship_options=&options,.overlay=&overlay,.projectiles=&projectiles,.ships=&ships,.hud_panels=&panels,.hud=&hud,.notices=&notices,.hud_background=231,.hud_highlight=123,.present=present,.user=&presentations};
    if(clear_surface(target)||clear_surface(reference))return 2;
    CHECK(sniper_scene_paint(&scene));if(presentations!=1||frame.stage||frame.flip||frame.restore||frame.user)return 3;
    order.frame=0;blink=0;
    CHECK(sniper_starfield_paint(&stars,reference,640,200,0,0,near_color,far_color));
    CHECK(sniper_map_paint(&map_painter,reference,map,0,0,640,200,0));
    CHECK(sniper_actors_paint(&actors,&order,&options,&overlay,&projectiles,&ships,reference,0));
    CHECK(sniper_hud_paint(&panels,reference,&hud,231,123));
    CHECK(sniper_notices_paint(&overlay,reference,&notices,640,200));
    DDSURFACEDESC a={0},b={0};a.dwSize=sizeof a;b.dwSize=sizeof b;CHECK(IDirectDrawSurface_Lock(target,NULL,&a,DDLOCK_WAIT,NULL));CHECK(IDirectDrawSurface_Lock(reference,NULL,&b,DDLOCK_WAIT,NULL));
    unsigned changed=0;for(int y=0;y<200;++y)for(int x=0;x<640;++x){uint8_t av=((uint8_t *)a.lpSurface)[y*a.lPitch+x],bv=((uint8_t *)b.lpSurface)[y*b.lPitch+x];if(av!=bv){printf("FAIL pixel%d,%d %u/%u\n",x,y,av,bv);return 4;}changed+=av!=0;}
    CHECK(IDirectDrawSurface_Unlock(target,a.lpSurface));CHECK(IDirectDrawSurface_Unlock(reference,b.lpSurface));if(changed<1000)return 5;
    frame.radar=1;if(sniper_scene_paint(&scene)!=E_NOTIMPL||presentations!=1)return 6;
    printf("PASS composed indexed scene: stars,map,pickup,bullet,ship,impact,HUD,notice;%u nonzero pixels;missing overlay blocks presentation\n",changed);
    if(!panels.team.surface||!panels.equipment.cache.surface)return 7;
    DDSURFACEDESC tile={0};tile.dwSize=sizeof tile;CHECK(IDirectDrawSurface_Lock(map_painter.ordinary.surface,NULL,&tile,DDLOCK_WAIT,NULL));
    uint8_t tile_first=*(uint8_t *)tile.lpSurface;*(uint8_t *)tile.lpSurface=(uint8_t)(tile_first+1);CHECK(IDirectDrawSurface_Unlock(map_painter.ordinary.surface,tile.lpSurface));
    RecoveryTest recovery={.scene=&scene,.palette=palette};
    scene.primary=reference;scene.present=loss_present;scene.recover=recover_scene;scene.user=&recovery;frame.radar=1;scene.extension=lost_stage;
    CHECK(sniper_scene_paint(&scene));
    if(recovery.calls!=2||recovery.recoveries!=1||panels.team.surface||panels.equipment.cache.surface)return 8;
    for(int i=0;i<3;++i)if(recovery.state.dirty[i]!=1)return 9;
    for(int i=0;i<16;++i)if(recovery.state.colors[i]>255)return 10;
    if(scene.near_color!=near_color||scene.far_color!=far_color||scene.hud_background!=recovery.state.colors[0]||scene.hud_highlight!=recovery.state.colors[9]||hud.energy_low!=recovery.state.colors[8]||hud.energy_high!=recovery.state.colors[10]||hud.energy_medium!=recovery.state.colors[11]||hud.border!=recovery.state.colors[3]||hud.capacity!=recovery.state.colors[12])return 11;
    CHECK(IDirectDrawSurface_Lock(map_painter.ordinary.surface,NULL,&tile,DDLOCK_WAIT,NULL));
    if(*(uint8_t *)tile.lpSurface!=tile_first)return 12;
    CHECK(IDirectDrawSurface_Unlock(map_painter.ordinary.surface,tile.lpSurface));
    frame.radar=0;scene.extension=NULL;
    CHECK(sniper_scene_paint(&scene));if(recovery.calls!=3||!panels.team.surface||!panels.equipment.cache.surface)return 13;
    puts("PASS injected presentation loss: actual surface/palette/tile restoration,16 color remaps,dirty flags,HUD cache release and next-frame rebuild");
    sniper_hud_panels_destroy(&panels);sniper_surface_destroy(&ships.body[0]);sniper_overlay_painter_destroy(&overlay);sniper_surface_destroy(&map_painter.ordinary);sniper_surface_destroy(&projectiles.assets[0]);sniper_surface_destroy(&projectiles.assets[2]);
    IDirectDrawSurface_Release(target);IDirectDrawSurface_Release(reference);IDirectDrawPalette_Release(palette);IDirectDraw_Release(dd);DestroyWindow(window);return 0;
}
