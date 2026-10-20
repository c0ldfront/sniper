#include "session_painter.h"
#include <string.h>
#include "../src/help_panel.h"
#include "../src/online_session.h"
static int recover(void *user) {
    SniperSessionPainter *p=user;int ok=sniper_scene_restore(&p->scene,p->graphics->palette,&p->recovery);
    if(ok){p->session->binding.list_dirty=1;p->session->radar_dirty=1;p->session->chat.dirty=1;}
    return ok;
}
static uint32_t chat_clock(void *user) {
    SniperOfflineSession *s=((SniperSessionPainter *)user)->session;return s->host.clock_ms(s->host.user);
}
static uint32_t hud_clock(void *user) {
    SniperOfflineSession *s=((SniperSessionPainter *)user)->session;
    return s->online?s->online->client->transport.host.clock_ms(s->online->client->transport.host.user):s->host.clock_ms(s->host.user);
}
static uint32_t hud_last_receive(void *user) {
    SniperOfflineSession *s=((SniperSessionPainter *)user)->session;
    return s->online?s->online->client->transport.last_receive_tick:0;
}
static void ticker(void *user){sniper_session_ticker(((SniperSessionPainter *)user)->session);}
static HRESULT extension(void *user,SniperSceneStage stage) {
    SniperSessionPainter *p=user;
    if(stage==SNIPER_SCENE_SPECIAL){
        SniperOfflineSession *s=p->session;SniperEditor *e=&s->editor;
        SniperEditorRenderState state={.cursor_x=e->x,.cursor_y=e->y,
            .camera_x=s->frame.camera.x,.camera_y=s->frame.camera.y,
            .screen_width=p->display->width,.screen_height=p->display->height,
            .selected_tile=e->tile,.selection_active=e->selection,.x1=e->x1,.y1=e->y1,.x2=e->x2,.y2=e->y2,
            .clipboard_active=e->clipboard!=NULL,.clipboard_width=e->clipboard_width,.clipboard_height=e->clipboard_height,
            .selection_color=p->graphics->colors[11],.clipboard_color=p->graphics->colors[10],
            .palette_background_color=p->graphics->colors[3]};
        return sniper_editor_paint(&p->graphics->editor,&p->graphics->map.ordinary,p->display->backbuffer,&state);
    }
    if(stage==SNIPER_SCENE_CHAT) {
        SniperOfflineSession *s=p->session;
        SniperChatPaint chat={.draw=p->display->draw,.target=p->display->backbuffer,
            .fonts=p->graphics->overlay.fonts,.lines=s->chat_lines,.colors=s->chat_colors,.input=s->chat.text,
            .width=p->display->width,.height=p->display->height,.rows=s->chat_rows,
            .dirty=&s->chat.dirty,.enabled=&s->chat.enabled,.ticker_enabled=s->ticker_enabled,
            .ticker_interval=s->ships.config.word[(0x4282e5-0x4281c9)/4],.background=p->graphics->colors[0],
            .milliseconds=chat_clock,.ticker=ticker,.recover=recover,.user=p};
        return sniper_chat_paint(&p->graphics->chat,&chat);
    }
    if(stage==SNIPER_SCENE_RADAR) {
        SniperOfflineSession *s=p->session;
        SniperRadarPaint radar={.draw=p->display->draw,.target=p->display->backbuffer,.map=s->map,.colors=p->graphics->colors,
            .width=p->display->width,.height=p->display->height,.camera_x=s->frame.camera.x,.camera_y=s->frame.camera.y,
            .scale=s->ships.config.word[(0x428279-0x4281c9)/4],.small=s->radar_small,
            .dirty=&s->radar_dirty,.enabled=&s->radar_enabled,.recover=recover,.user=p};
        radar.markers=(SniperRadarMarkers){.local=sniper_session_local(s),.reference=sniper_session_local(s)->word[0x40/4]==4?s->frame.watched:sniper_session_local(s),.ships=s->ordered,
            .pickups=s->pickup_items,.projectiles=s->combat.projectiles,.ship_count=s->ships.player_count,
            .pickup_count=s->pickups.collection.count,.projectile_count=s->combat.count,.local_id=s->start.local_id,.watched_id=s->start.watched,
            .special=s->frame.special,.tile_x=s->frame.tile_x,.tile_y=s->frame.tile_y,
            .bounty_upper=s->ships.config.word[(0x4282bd-0x4281c9)/4],.bounty_lower=s->ships.config.word[(0x4282c1-0x4281c9)/4],
            .bounty_bonus=s->ships.config.word[(0x4282c5-0x4281c9)/4]};
        memcpy(&radar.markers.tick,&s->frame.clock.tick,4);
        return sniper_radar_paint(&p->graphics->radar,&radar);
    }
    if(stage==SNIPER_SCENE_MODE_TWO||stage==SNIPER_SCENE_MODE_FOUR) {
        SniperOfflineSession *s=p->session;SniperStatisticsPanel *content=&p->graphics->statistics_content;
        int updated=sniper_statistics_update(content,s->ordered,s->ships.player_count,sniper_session_local(s),
            s->start.history_size,&s->binding.selected,&s->binding.list_dirty,stage==SNIPER_SCENE_MODE_FOUR,
            s->host.clock_ms,s->host.user);
        if(updated<0)return E_INVALIDARG;
        if(updated)sniper_text_panel_close(&p->graphics->statistics);
        return sniper_text_panel_paint(&p->graphics->statistics,p->display->draw,p->display->backbuffer,
            p->graphics->overlay.fonts,content->title,content->rows,content->count,content->per,0,8,
            p->graphics->colors[0],p->graphics->colors[15],recover,p);
    }
    if(stage!=SNIPER_SCENE_MODE_ONE&&stage!=SNIPER_SCENE_MODE_THREE)return E_NOTIMPL;
    int32_t count;const SniperPanelRow *rows=sniper_help_rows(stage==SNIPER_SCENE_MODE_THREE,&count);
    return sniper_text_panel_paint(&p->graphics->help,p->display->draw,p->display->backbuffer,
        p->graphics->overlay.fonts,"Help v.87",rows,count,50,8,8,p->graphics->colors[0],p->graphics->colors[15],recover,p);
}
void sniper_session_painter_init(SniperSessionPainter *p,SniperOfflineSession *s,SniperGameDisplay *d,SniperGameGraphics *g) {
    memset(p,0,sizeof *p);p->session=s;p->display=d;p->graphics=g;
    p->frame.local=sniper_session_local(s);
    p->actors=(SniperActorLayers){.pickups=s->pickup_items,.pickup_count=&s->pickups.collection.count,
        .projectiles=s->combat.projectiles,.projectile_count=&s->combat.count,
        .camera_x=&s->frame.camera.x,.camera_y=&s->frame.camera.y};
    p->order.ships=s->ordered;p->order.local=sniper_session_local(s);p->order.local_id=s->start.local_id;
    p->options.players=s->ships.players;p->options.blink=&p->blink;
    p->hud.energy_low=g->colors[8];p->hud.energy_medium=g->colors[11];p->hud.energy_high=g->colors[10];
    p->hud.border=g->colors[3];p->hud.capacity=g->colors[12];
    p->scene=(SniperScenePainter){.frame=&p->frame,.target=d->backbuffer,.primary=d->primary,
        .width=d->width,.height=d->height,.camera_x=&s->frame.camera.x,.camera_y=&s->frame.camera.y,
        .map=s->map,.stars=&s->stars,.near_color=g->colors[13],.far_color=g->colors[14],
        .map_painter=&g->map,.actors=&p->actors,.ship_order=&p->order,.ship_options=&p->options,
        .overlay=&g->overlay,.projectiles=&g->projectiles,.ships=&g->ships,.hud_panels=&g->panels,
        .hud=&p->hud,.notices=&s->ships.attachment_hud,.hud_background=g->colors[0],
        .hud_highlight=g->colors[9],.extension=extension,.recover=recover,.user=p};
}
HRESULT sniper_session_present(SniperSessionPainter *p) {
    SniperOfflineSession *s=p->session;
    DDBLTFX fill={0};fill.dwSize=sizeof fill;fill.dwFillColor=p->scene.hud_background;
    RECT rect={0,0,p->display->width,p->display->height};HRESULT hr;
    do {hr=IDirectDrawSurface_Blt(p->display->backbuffer,&rect,NULL,NULL,DDBLT_COLORFILL,&fill);}while(hr==DDERR_WASSTILLDRAWING);
    if(hr==DDERR_SURFACELOST){if(!recover(p))return hr;}
    else if(FAILED(hr))return hr;
    s->mouse.present=p->graphics->statistics_content.valid;
    s->mouse.width=p->graphics->statistics.layout.width;s->mouse.height=p->graphics->statistics.layout.height;
    s->mouse.scroll=p->graphics->statistics_content.scroll;
    if(!sniper_session_step(s))return E_NOTIMPL;
    p->frame.stars=s->stars_enabled;p->options.label_mode=s->label_mode;
    p->options.upper=s->ships.config.word[(0x4282bd-0x4281c9)/4];
    p->options.lower=s->ships.config.word[(0x4282c1-0x4281c9)/4];
    p->options.bonus=s->ships.config.word[(0x4282c5-0x4281c9)/4];
    p->frame.local=sniper_session_local(s);p->frame.special=s->frame.special;
    p->frame.mode=s->overlay_mode;p->frame.radar=s->radar_enabled;p->frame.chat=s->chat.enabled;
    memcpy(&p->scene.tick,&s->frame.clock.tick,4);
    p->order.local=p->frame.local;p->order.local_id=s->start.local_id;p->order.count=s->ships.player_count;
    p->hud.maximum_energy=s->ships.config.word[(0x428239-0x4281c9)/4];
    p->hud.safety_ticks=s->combat.safe_timer;p->hud.player_count=s->ships.player_count;
    p->hud.connected=s->online!=NULL;p->hud.milliseconds=hud_clock;
    p->hud.last_receive_tick=hud_last_receive;p->hud.user=p;
    return sniper_scene_paint(&p->scene);
}
