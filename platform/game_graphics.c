#include "game_graphics.h"
#include "palette_resource.h"
#include "../src/display_recovery.h"
#include <string.h>
void sniper_game_graphics_close(SniperGameGraphics *g) {
    sniper_editor_painter_close(&g->editor);
    sniper_radar_close(&g->radar);sniper_chat_close(&g->chat);
    sniper_text_panel_close(&g->help);
    sniper_text_panel_close(&g->statistics);
    sniper_hud_panels_destroy(&g->panels);
    sniper_overlay_painter_destroy(&g->overlay);
    sniper_projectile_painter_destroy(&g->projectiles);
    sniper_ship_painter_destroy(&g->ships);
    sniper_map_painter_destroy(&g->map);
    if(g->palette)IDirectDrawPalette_Release(g->palette);
    memset(g,0,sizeof *g);
}
static HRESULT palette_sprite(SniperSpriteSurface *s,LPDIRECTDRAWPALETTE p) {
    HRESULT hr=IDirectDrawSurface_SetPalette(s->surface,p);
    /* Reload with the assigned palette so GDI color conversion is explicit. */
    return FAILED(hr)?hr:sniper_surface_restore(s);
}
HRESULT sniper_game_graphics_open(SniperGameGraphics *g,SniperGameDisplay *d,HINSTANCE instance) {
    if(g->palette||!d->draw||!d->primary||!d->backbuffer)return E_INVALIDARG;
    HRESULT hr=sniper_palette_create_resource(d->draw,instance,107,&g->palette);
    if(FAILED(hr))goto fail;
    hr=IDirectDrawSurface_SetPalette(d->primary,g->palette);if(FAILED(hr))goto fail;
    hr=IDirectDrawSurface_SetPalette(d->backbuffer,g->palette);if(FAILED(hr))goto fail;
    hr=sniper_map_painter_create(&g->map,d->draw,instance);if(FAILED(hr))goto fail;
    hr=sniper_ship_painter_create(&g->ships,d->draw,instance);if(FAILED(hr))goto fail;
    hr=sniper_projectile_painter_create(&g->projectiles,d->draw,instance);if(FAILED(hr))goto fail;
    hr=sniper_overlay_painter_create(&g->overlay,d->draw,instance);if(FAILED(hr))goto fail;
    SniperSpriteSurface *groups[]={&g->map.ordinary,g->map.special,g->ships.body,
        g->ships.effects,g->ships.fonts,g->projectiles.assets,&g->overlay.pickup,g->overlay.fonts};
    const int counts[]={1,4,9,2,4,5,1,4};
    for(int group=0;group<8;++group)for(int i=0;i<counts[group];++i) {
        hr=palette_sprite(groups[group]+i,g->palette);if(FAILED(hr))goto fail;
    }
    for(int i=0;i<16;++i) {
        hr=sniper_indexed_color(g->map.ordinary.surface,sniper_display_color_rgb[i],g->colors+i);
        if(FAILED(hr))goto fail;
    }
    hr=sniper_editor_painter_create(&g->editor,d->draw,instance,g->palette);if(FAILED(hr))goto fail;
    sniper_hud_panels_init(&g->panels,d->draw,g->overlay.fonts);
    return DD_OK;
fail:
    sniper_game_graphics_close(g);return hr;
}
