#include "display_recovery_painter.h"
static uint32_t restore(void *user,uintptr_t s)
{
    (void)user;return (uint32_t)IDirectDrawSurface_Restore((LPDIRECTDRAWSURFACE)s);
}
static void release(void *user,uintptr_t s)
{
    (void)user;IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)s);
}
static uint32_t set_palette(void *user,uintptr_t s,uintptr_t p)
{
    (void)user;return (uint32_t)IDirectDrawSurface_SetPalette((LPDIRECTDRAWSURFACE)s,(LPDIRECTDRAWPALETTE)p);
}
static uint32_t tiles(void *user,uintptr_t object)
{
    (void)user;return (uint32_t)sniper_surface_restore((SniperSpriteSurface *)object);
}
static uint32_t color(void *user,uintptr_t surface,uint32_t rgb)
{
    (void)user;uint8_t index;
    HRESULT hr=sniper_indexed_color((LPDIRECTDRAWSURFACE)surface,(COLORREF)rgb,&index);
    return FAILED(hr)?UINT32_MAX:index;
}
int sniper_scene_restore(SniperScenePainter *p,LPDIRECTDRAWPALETTE palette,SniperDisplayRecovery *s)
{
    if(!p||!s||!p->primary||!p->target||!p->hud_panels||!p->hud||!p->map_painter)return 0;
    s->primary=(uintptr_t)p->primary;s->backbuffer=(uintptr_t)p->target;
    s->team_panel=p->hud_panels->team.surface;s->equipment_panel=p->hud_panels->equipment.cache.surface;
    s->palette=(uintptr_t)palette;s->tile_object=(uintptr_t)&p->map_painter->ordinary;
    s->tile_surface=(uintptr_t)p->map_painter->ordinary.surface;
    s->restore=restore;s->release=release;s->set_palette=set_palette;s->restore_tiles=tiles;s->resolve_color=color;s->user=NULL;
    int ok=sniper_display_recover(s);
    p->hud_panels->team.surface=s->team_panel;p->hud_panels->equipment.cache.surface=s->equipment_panel;
    if(ok){
        p->hud_background=s->colors[0];p->hud_highlight=s->colors[9];
        p->hud->energy_low=s->colors[8];p->hud->energy_medium=s->colors[11];p->hud->energy_high=s->colors[10];
        p->hud->border=s->colors[3];p->hud->capacity=s->colors[12];
        p->near_color=(uint8_t)s->colors[13];p->far_color=(uint8_t)s->colors[14];
    }
    return ok;
}
