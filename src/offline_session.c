#include "offline_session.h"
#include <stdlib.h>
#include <string.h>
#include "exit_request.h"
#include "capability_input.h"
#include "manual_warp.h"
#include "attachment_command.h"
#include "list_navigation.h"
#include "help_panel.h"
#include "statistics_panel.h"
#include "chat_dispatch.h"
#include "chat_history.h"
#include "chat_tips.h"
#include "offline_team.h"
#include "spectator_control.h"
#include "ship_spawn.h"
#include "online_session.h"
#include "inactive_position.h"
#include "voice_input.h"
#include "display_input.h"
SniperShipObject *sniper_session_local(SniperOfflineSession *s){int id=s->start.local_id;return id>=0&&id<255?s->ships.players[id]:NULL;}
static uint32_t clock_ms(void *user){SniperOfflineSession *s=user;return s->host.clock_ms(s->host.user);}
static int32_t key(void *user,int32_t vk){SniperOfflineSession *s=user;return s->host.key_state(s->host.user,vk);}
static uint32_t joystick(void *user,uint32_t id,uint32_t info[13]) {
    SniperOfflineSession *s=user;return s->host.joystick?s->host.joystick(s->host.user,id,info):165;
}
static void post(void *user){SniperOfflineSession *s=user;if(s->host.post_command)s->host.post_command(s->host.user);}
static void cursor(void *user,int32_t x,int32_t y){SniperOfflineSession *s=user;if(s->host.cursor)s->host.cursor(s->host.user,x,y);}
static void sound(void *user,int32_t handle,int32_t x,int32_t y){
    SniperOfflineSession *s=user;SniperShipObject *local=sniper_session_local(s);
    if(s->host.sound)s->host.sound(s->host.user,handle,x,y);
    SniperAudioRequest request;
    if(sniper_audio_position(x,y,local?local->word[0x48/4]:0,local?local->word[0x4c/4]:0,
        local!=NULL,s->frame.width,&request)) {
        int32_t spread=s->ships.config.word[(0x4282a5-0x4281c9)/4];
        request.pitch=sniper_audio_pitch((int32_t)sniper_random(&s->ships.random_state),spread);
        if(s->host.audio)s->host.audio(s->host.user,handle,&request);
    }
}
static void impact(void *user,int32_t x,int32_t y){sound(user,134,x,y);}
static void prize_sound(void *user,int32_t handle){
    SniperOfflineSession *s=user;SniperAudioRequest request={0};
    if(s->host.sound)s->host.sound(s->host.user,handle,0,0);
    if(s->host.audio)s->host.audio(s->host.user,handle,&request);
}
static void engine_stop(SniperOfflineSession *s){
    if(s->host.audio_stop){s->host.audio_stop(s->host.user,125);s->host.audio_stop(s->host.user,127);s->host.audio_stop(s->host.user,169);}
}
static void flight_audio(void *user,const SniperFlightEffects *effects){
    SniperOfflineSession *s=user;const int resources[3]={125,127,169};SniperAudioRequest request={0,0,1,0};
    for(int i=0;i<effects->sound_count;i++){
        int resource=resources[effects->sound_slot[i]];
        if(effects->sound_play[i]){if(s->host.audio)s->host.audio(s->host.user,resource,&request);}
        else if(s->host.audio_stop)s->host.audio_stop(s->host.user,resource);
    }
}
static SniperShipObject *allocate_ship(void *user){(void)user;return calloc(1,sizeof(SniperShipObject));}
static void release_ship(void *user,SniperShipObject *ship){(void)user;free(ship);}
static SniperProjectile *allocate_projectile(void *user){(void)user;return calloc(1,sizeof(SniperProjectile));}
static void release_projectile(void *user,SniperProjectile *p){(void)user;free(p);}
static SniperPickupObject *allocate_pickup(void *user){(void)user;return calloc(1,sizeof(SniperPickupObject));}
static void release_pickup(void *user,SniperPickupObject *p){(void)user;free(p);}
static void attach_command(void *user,int32_t target) {
    SniperOfflineSession *s=user;
    if(s->online)sniper_network_client_attach(s->online->client,target);
    else sniper_ship_world_offline_attach(&s->ships,target);
}
static void release_children(void *user){SniperOfflineSession *s=user;if(s->online)sniper_network_client_release_children(s->online->client,sniper_session_local(s)->word[0x68/4]);}
static void attachment_sound(void *user) {
    SniperOfflineSession *s=user;prize_sound(s,s->ships.warp_sound);
}
static void chat_send(void *user,int32_t recipient,const char *text) {
    SniperOfflineSession *s=user;
    if(s->online)sniper_network_client_chat(s->online->client,recipient,text,(uint32_t)s->ships.config.word[(0x4282d9-0x4281c9)/4]);
    else sniper_chat_history(s->chat_lines,s->chat_colors,s->chat_rows,s->frame.width,
        &s->chat.dirty,text,(const char *)sniper_session_local(s),recipient);
}
static void chat_team(void *user,int32_t frequency) {
    SniperOfflineSession *s=user;
    if(s->online)sniper_network_client_team(s->online->client,frequency);
    else sniper_offline_team(&s->ships,s->ordered,&s->binding.selected,&s->binding.list_dirty,frequency);
}
static void chat_submit(void *user,const char *text) {
    SniperOfflineSession *s=user;
    SniperChatDispatch dispatch={.local=sniper_session_local(s),.ordered=s->ordered,
        .count=s->ships.player_count,.selected=s->binding.selected,
        .team_modulus=s->ships.config.word[(0x4282a9-0x4281c9)/4],
        .team_capacity=s->ships.config.word[(0x4282b1-0x4281c9)/4],
        .requested_team=&s->frame.frequency,.notices=&s->ships.attachment_hud,
        .send=chat_send,.team=chat_team,.attach=attach_command,.user=s};
    sniper_chat_dispatch(&dispatch,text);
}
void sniper_session_ticker(SniperOfflineSession *s) {
    sniper_chat_history(s->chat_lines,s->chat_colors,s->chat_rows,s->frame.width,
        &s->chat.dirty,sniper_chat_tips[s->ticker_index],NULL,0);
    s->ticker_index=(s->ticker_index+1)%33;
}
/* Offline virtual+0c411530 and +204115d0 are return-only. */
static void spectator_position(void *user,const int32_t args[13]){SniperOfflineSession *s=user;if(s->online)sniper_network_client_position(s->online->client,args);}
static void spectator_watch(void *user,int32_t id){SniperOfflineSession *s=user;if(s->online)sniper_network_client_ship(s->online->client,id);}
static int32_t editor_event_key(void *user,int32_t vk){
    SniperOfflineSession *s=user;return s->host.key_state_event?s->host.key_state_event(s->host.user,vk):key(s,vk);
}
static void editor_place(void *user,int32_t x,int32_t y,int32_t tile){
    SniperOfflineSession *s=user;
    if(sniper_map_place(s->map,s->editor_sizes,(unsigned)x,(unsigned)y,(uint8_t)tile)||
       !sniper_editor_records_place(&s->editor_records,x,y,tile))s->editor_error=1;
    /* Native411580 does not dirty the cached radar on a map edit. */
}
static void voice_send(void *user,int32_t recipient,int32_t slot,const void *data,int32_t length){
    SniperOfflineSession *s=user;
    if(s->online)sniper_network_client_cached(s->online->client,recipient,slot,data,(size_t)length);
    else if(data&&s->voice_receive)s->voice_receive(s->voice_user,0,data,(size_t)length);
}
static void input(void *user,SniperOfflineFrame *binding) {
    SniperOfflineSession *s=user;
    if(!s->active){s->position.local=sniper_session_local(s);s->position.position_dirty=s->ships.position_dirty;
        s->position.send=spectator_position;s->position.user=s;sniper_inactive_position(&s->position);s->ships.position_dirty=s->position.position_dirty;return;}
    s->mouse.left_held=key(s,1)&0x8000;
    int32_t right=key(s,2)&0x8000;
    if(s->mouse.left_held||right)s->start.cursor_tick=clock_ms(s)/10u;
    s->mouse.left_event=s->lifecycle.mouse_x_event;
    s->mouse.repeat=sniper_mouse_repeat(&s->mouse_repeat_ticks,(int32_t)s->frame.clock.elapsed,
        s->lifecycle.mouse_x_event,s->lifecycle.mouse_y_event);
    if(s->frame.special){
        SniperEditorInput input={.key=s->lifecycle.key,.repeat=s->lifecycle.repeat,
            .radar=s->radar_enabled,.radar_dirty=s->radar_dirty,.character=(uint8_t)s->lifecycle.character};
        SniperEditorHost host={key,editor_event_key,editor_place,post,s};
        if(!sniper_editor_input(&s->editor,&input,s->map,s->editor_sizes,&host))s->editor_error=1;
        s->radar_enabled=input.radar;s->radar_dirty=input.radar_dirty;
        s->frame.tile_x=s->editor.x;s->frame.tile_y=s->editor.y;
        s->lifecycle.key=0;s->lifecycle.repeat=0;s->lifecycle.character=0;
        s->lifecycle.mouse_x_event=0;s->lifecycle.mouse_y_event=0;
        return;
    }
    int32_t chat_shift=s->host.key_state_event?s->host.key_state_event(s->host.user,16):key(s,16);
    if(sniper_chat_input(&s->chat,s->frame.width,s->lifecycle.character,s->lifecycle.key,
        chat_shift,chat_submit,s)){s->lifecycle.key=0;s->lifecycle.repeat=0;}
    SniperVoiceInput voice={.ordered=s->ordered,.count=s->ships.player_count,
        .selected=s->binding.selected,.enabled=s->ships.config.word[(0x4282e1-0x4281c9)/4],
        .transport_present=s->online!=NULL,
        .large_transfer_count=s->online?s->online->client->transport.sender.large_count:0,
        .recordings={s->voice_recordings.data[0],s->voice_recordings.data[1]},
        .lengths={s->voice_recordings.length[0],s->voice_recordings.length[1]},
        .notices=&s->ships.attachment_hud,.send=voice_send,.user=s};
    if(sniper_voice_input(&voice,s->lifecycle.key,chat_shift)){s->lifecycle.key=0;s->lifecycle.repeat=0;}
    sniper_display_input(s->lifecycle.key,&s->label_mode,&s->stars_enabled,&s->ticker_enabled);

    if(s->lifecycle.key==27)sniper_exit_request(&s->overlay_mode,&s->frame.auto_pending,&s->ships.attachment_hud);
    if(s->lifecycle.key==0x23 || s->lifecycle.key==0x24 || s->lifecycle.key==0x2e) {
        int32_t shift=0;
        if(s->lifecycle.key!=0x2e)shift=s->host.key_state_event?
            s->host.key_state_event(s->host.user,16):key(s,16);
        sniper_capability_input(sniper_session_local(s),s->lifecycle.key,shift,&s->ships.position_dirty);
    }
    if(s->lifecycle.key==0x2d)sniper_manual_warp(&s->ships,sniper_session_local(s));
    if(s->lifecycle.key==0x21 || s->lifecycle.key==0x22) {
        int32_t shift=s->host.key_state_event?s->host.key_state_event(s->host.user,16):key(s,16);
        sniper_list_navigation(s->lifecycle.key,shift,s->ships.player_count,s->frame.height,
            &s->binding.selected,&s->start.history_size,&s->binding.list_dirty);
    }
    if(s->lifecycle.key==0x77 && !(chat_shift&0x8000)) {
        s->control.engine_sound=!s->control.engine_sound;
        if(!s->control.engine_sound)engine_stop(s);
    }
    if(s->lifecycle.key==0x73) {
        int32_t shift=s->host.key_state_event?s->host.key_state_event(s->host.user,16):key(s,16);
        if(!(shift&0x8000)){s->radar_enabled=!s->radar_enabled;s->radar_dirty=1;}
    }
    if(s->lifecycle.key==0x71) {
        int32_t shift=s->host.key_state_event?s->host.key_state_event(s->host.user,16):key(s,16);
        if(!(shift&0x8000))s->overlay_mode=sniper_statistics_toggle(s->overlay_mode,&s->binding.list_dirty);
    }
    if(s->lifecycle.key==0x70) {
        int32_t shift=s->host.key_state_event?s->host.key_state_event(s->host.user,16):key(s,16);
        if(!(shift&0x8000))s->overlay_mode=sniper_help_toggle(s->overlay_mode,sniper_session_local(s)->word[0x40/4]);
    }
    if(s->lifecycle.key==0x76) {
        int32_t shift=s->host.key_state_event?s->host.key_state_event(s->host.user,16):key(s,16);
        if(!(shift&0x8000)) {
            SniperAttachmentCommand command={sniper_session_local(s),s->ordered,s->ships.player_count,
                s->binding.selected,s->start.local_id,&s->ships.attachment_hud,attach_command,release_children,attachment_sound,s};
            sniper_attachment_command(&command);
        }
    }
    s->mouse.count=s->ships.player_count;s->mouse.selected=s->binding.selected;
    s->mouse.history=s->start.history_size;s->mouse.dirty=s->binding.list_dirty;s->mouse.screen_height=s->frame.height;
    sniper_list_mouse(&s->mouse);
    s->binding.selected=s->mouse.selected;s->start.history_size=s->mouse.history;s->binding.list_dirty=s->mouse.dirty;
    s->control.input.mouse_pressed=s->mouse.left_held;s->control.input.mouse_x=s->mouse.x;s->control.input.mouse_y=s->mouse.y;
    if(sniper_session_local(s)->word[0x40/4]==4) {
        SniperSpectatorControl spectator={.input=&s->lifecycle,.ordered=s->ordered,
            .count=s->ships.player_count,.selected=s->binding.selected,
            .last_tick=s->position.last_ticks,.watched=s->start.watched,
            .position=spectator_position,.watch=spectator_watch,.user=s};
        memcpy(&spectator.tick,&s->frame.clock.tick,4);sniper_spectator_control(&spectator);
        s->position.last_ticks=spectator.last_tick;s->start.watched=spectator.watched;
        s->ships.movement.watched_id=s->start.watched;
        s->frame.watched=s->start.watched>=0&&s->start.watched<255?s->ships.players[s->start.watched]:NULL;
    } else if(s->online)sniper_online_gameplay_control(&s->online->gameplay,binding->control);
    else sniper_offline_control(binding->combat,binding->control);
}
void sniper_session_destroy(SniperOfflineSession *s) {
    if(!s)return;
    if(s->host.process_random_state)*s->host.process_random_state=s->ships.random_state;
    if(s->host.process_clock)*s->host.process_clock=s->frame.clock;
    engine_stop(s);
    sniper_online_session_release(s);
    sniper_voice_recordings_close(&s->voice_recordings);
    sniper_editor_close(&s->editor);sniper_editor_records_free(&s->editor_records);
    for(int32_t i=0;i<s->combat.count;++i)free(s->combat.projectiles[i]);
    for(int32_t i=0;i<s->pickups.collection.count;++i)free(s->pickup_items[i]);
    for(int32_t i=0;i<s->ships.player_count;++i)free(s->ordered[i]);
    free(s);
}
SniperOfflineSession *sniper_session_prepare(const SniperSessionHost *host,int32_t width,int32_t height,uint32_t seed){
    if(!host||!host->clock_ms||!host->key_state||width<=0||height<=0)return NULL;
    SniperOfflineSession *s=calloc(1,sizeof *s);if(!s)return NULL;
    s->host=*host;s->ships.random_state=host->process_random_state?*host->process_random_state:seed;s->active=1;
    if(host->process_clock){
        sniper_frame_clock_begin_session(&s->frame.clock,host->process_clock);
        *host->process_clock=s->frame.clock;
    }
    s->ships.clock_ms=clock_ms;s->ships.clock_user=s;
    sniper_voice_recordings_load(&s->voice_recordings,NULL);
    s->radar_enabled=1;s->radar_dirty=1;
    s->chat.enabled=1;s->chat.dirty=1;s->chat_rows=4;s->ticker_enabled=1;
    s->label_mode=1;s->stars_enabled=1;
    s->overlay_mode=4;
    sniper_starfield_generate(&s->stars,&s->ships.random_state);
    if(host->process_random_state)*host->process_random_state=s->ships.random_state;
    s->ships.movement.map=s->map;s->ships.offline_backend=1;
    s->ships.movement.impact_sound=impact;s->ships.movement.user=s;
    s->ships.backend=s;s->ships.prize_sound=prize_sound;
    s->ships.positive_sound=143;s->ships.negative_sound=121;s->ships.warp_sound=148;
    sniper_local_config(&s->ships.config);
    s->start.local_id=-1;s->start.watched=-1;s->start.height=height;s->start.history_size=20;s->start.cursor=cursor;s->start.user=s;
    s->combat.bullet_sounds[0]=162;s->combat.bullet_sounds[1]=164;s->combat.bullet_sounds[2]=165;
    s->combat.bomb_sounds[0]=166;s->combat.bomb_sounds[1]=167;s->combat.bomb_sounds[2]=168;
    s->combat.impact_sound=121;s->combat.death_sound=123;s->combat.explosion_sound=124;s->combat.repel_sound=177;
    s->combat.ships=&s->ships;s->combat.ordered_ships=s->ordered;
    s->combat.allocate=allocate_projectile;s->combat.release=release_projectile;s->combat.sound=sound;s->combat.user=s;
    s->pickups.ships=&s->ships;s->pickups.collection.items=s->pickup_items;s->pickups.movement_view=s->pickup_view;
    s->pickups.allocate=allocate_pickup;s->pickups.release=release_pickup;s->pickups.owner=s;
    s->pickups.clock_ms=clock_ms;s->pickups.clock_user=s;
    s->lifecycle.poll_joystick=joystick;s->lifecycle.user=s;
    s->control.engine_sound=1;s->control.audio_effects=flight_audio;s->control.audio_user=s;
    s->control.lifecycle=&s->lifecycle;s->control.position=&s->position;
    s->control.input.key_state=key;s->control.input.user=s;s->control.input.vertical_extent=height;
    s->frame.width=width;s->frame.height=height;
    s->frame.post_command=post;s->frame.user=s;
    s->binding.frame=&s->frame;s->binding.combat=&s->combat;s->binding.pickups=&s->pickups;s->binding.control=&s->control;
    s->binding.input=input;s->binding.input_user=s;
    s->binding.selected=s->start.selected;s->binding.list_dirty=s->start.list_dirty;
    return s;
}
void sniper_session_bind_local(SniperOfflineSession *s,SniperShipObject *local,int32_t id){
    s->start.local_id=id;s->start.local=local;s->ships.movement.local_id=id;
    s->lifecycle.local=local;s->lifecycle.local_id=id;s->frame.local=local;s->frame.frequency=s->start.frequency;
    s->frame.watched=s->start.watched>=0&&s->start.watched<255?s->ships.players[s->start.watched]:NULL;
    s->binding.selected=s->start.selected;s->binding.list_dirty=s->start.list_dirty;
    if(!s->host.process_clock)sniper_frame_clock_rebase(&s->frame.clock,clock_ms(s));
    if(s->host.process_random_state)*s->host.process_random_state=s->ships.random_state;
}
SniperOfflineSession *sniper_session_create(const SniperSessionHost *host,const uint8_t *level,size_t bytes,
    const SniperTileSize sizes[256],const char *name,int32_t type,int32_t team,
    int32_t width,int32_t height,uint32_t seed) {
    if(!name||!sizes||(!level&&bytes))return NULL;
    SniperOfflineSession *s=sniper_session_prepare(host,width,height,seed);if(!s)return NULL;
    uint32_t generation_ms=clock_ms(s);
    SniperPlayerRegistry registry={.players=s->ships.players,.ordered=s->ordered,
        .notices=&s->ships.attachment_hud,.clock_ms=clock_ms,.clock_user=s,
        .allocate=allocate_ship,.release=release_ship,.user=s};
    SniperShipObject *local=sniper_offline_init(&registry,&s->ships.config,s->map,sizes,
        level,bytes,generation_ms,&s->pickups.generation_tick,name,type,team,0);
    s->ships.player_count=registry.count;
    if(!local){sniper_session_destroy(s);return NULL;}
    s->ships.movement.bounce_divisor=sniper_local_bounce(&s->ships.config);
    s->start.height=height;s->start.history_size=20;s->start.cursor=cursor;s->start.user=s;
    sniper_offline_start(&s->ships,s->ordered,&s->start,0);
    sniper_offline_pickups_bind(&s->pickups);
    sniper_session_bind_local(s,local,0);
    return s;
}
SniperOfflineSession *sniper_session_create_editor(const SniperSessionHost *host,const uint8_t *level,size_t bytes,
    const SniperTileSize sizes[256],int32_t width,int32_t height,uint32_t seed,
    int32_t cursor_x,int32_t cursor_y,int32_t selected_tile){
    if(cursor_x<0||cursor_x>=1024||cursor_y<0||cursor_y>=1024||selected_tile<1||selected_tile>152)return NULL;
    SniperOfflineSession *s=sniper_session_create(host,level,bytes,sizes,"",0,0,width,height,seed);
    if(!s)return NULL;
    if(!sniper_editor_records_load(&s->editor_records,level,bytes)){sniper_session_destroy(s);return NULL;}
    memcpy(s->editor_sizes,sizes,sizeof s->editor_sizes);
    s->editor.x=cursor_x;s->editor.y=cursor_y;s->editor.tile=selected_tile;
    s->frame.special=1;s->frame.tile_x=cursor_x;s->frame.tile_y=cursor_y;
    s->overlay_mode=0;s->radar_enabled=0;s->chat.enabled=0;
    return s;
}
int sniper_session_step(SniperOfflineSession *s) {
    s->ships.movement.camera_x=s->frame.camera.x;s->ships.movement.camera_y=s->frame.camera.y;
    int ok=sniper_offline_frame(&s->binding,clock_ms(s));
    if(s->host.process_clock)*s->host.process_clock=s->frame.clock;
    if(s->host.process_random_state)*s->host.process_random_state=s->ships.random_state;
    return ok&&!s->editor_error&&(!s->online||!s->online->failed);
}
