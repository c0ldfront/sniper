#include "online_session.h"
#include "level.h"
#include <stdlib.h>
#include <string.h>
static uint32_t clock_ms(void*u){SniperOnlineSession*o=u;return o->session->host.clock_ms(o->session->host.user);}
static SniperShipObject*allocate_ship(void*u){(void)u;return calloc(1,sizeof(SniperShipObject));}
static void release_ship(void*u,SniperShipObject*p){(void)u;free(p);}
static SniperPickupObject*allocate_pickup(void*u){(void)u;return calloc(1,sizeof(SniperPickupObject));}
static void detach(void*u,int32_t target){SniperOnlineSession*o=u;sniper_network_client_attach(o->client,target);}
static void world_detach(void*u,int32_t target){SniperOfflineSession*s=u;detach(s->online,target);}
static void spawn(void*u,SniperShipObject*p,int reset,int move){SniperOnlineSession*o=u;SniperOfflineSession*s=o->session;sniper_ship_spawn(p->word,&o->defaults,s->map,s->ships.player_count,clock_ms(o),&s->ships.random_state,reset,move,&s->ships.position_dirty);}
static void damage(void*u,SniperShipObject*p,int32_t amount,int32_t owner){SniperOnlineSession*o=u;sniper_online_gameplay_damage(&o->gameplay,p,amount,owner);}
static void weapon(void*u,const int32_t*a){SniperOnlineSession*o=u;sniper_online_gameplay_weapon(&o->gameplay,a);}
static void extension(void*u,const SniperGameEvent*e){SniperOnlineSession*o=u;if(e->kind==SNIPER_NET_DATA&&o->session->voice_receive)o->session->voice_receive(o->session->voice_user,e->args[0],e->data,e->size);if(o->previous_event)o->previous_event(o->previous_event_user,e);}
static void settings(SniperOnlineSession*o){
    SniperOfflineSession*s=o->session;
    if(o->client->game.settings[0]==17)memcpy(&s->ships.config,o->client->game.settings+1,sizeof(s->ships.config));
    sniper_local_ship_defaults(&s->ships.config,&o->defaults);
    o->death.pickup_lifetime=s->ships.config.word[(0x428211-0x4281c9)/4];
    o->death.kill_bounty=s->ships.config.word[(0x42826d-0x4281c9)/4];
}
static void event(void*u,const SniperGameEvent*e){
    SniperOnlineSession*o=u;SniperOfflineSession*s=o->session;
    if(o->accept_event&&!o->accept_event(o->accept_event_user,e))return;
    settings(o);
    o->registry.selected=s->binding.selected;o->registry.dirty=s->binding.list_dirty;
    if(e->kind==SNIPER_NET_LEAVE&&o->ready&&e->args[0]==s->start.local_id){o->failed=1;return;}
    if(e->kind==SNIPER_NET_JOIN&&e->args[0]>=0&&e->args[0]<255&&s->ships.players[e->args[0]]){o->failed=1;return;}
    sniper_network_backend_event(&o->backend,e);
    s->ships.player_count=o->registry.count;s->binding.selected=o->registry.selected;s->binding.list_dirty=o->registry.dirty;
    sniper_online_gameplay_sync_pickups(&o->gameplay);
}
static void backend(void*u){SniperOnlineSession*o=u;(void)sniper_online_session_poll(o->session);}
static void team(void*u,int32_t n){SniperOnlineSession*o=u;sniper_network_client_team(o->client,n);}
static void simulate(void*u,const int32_t*ticks){SniperOnlineSession*o=u;if(!o->failed)sniper_online_gameplay_simulation(&o->gameplay,ticks);}
SniperOfflineSession *sniper_online_session_begin(const SniperSessionHost*h,SniperNetworkClient*c,const uint8_t*level,size_t bytes,const SniperTileSize sizes[256],int32_t ship,int32_t frequency,int32_t audio,int32_t width,int32_t height,uint32_t seed){
    if(!c||!sizes||(!level&&bytes))return NULL;
    SniperOfflineSession*s=sniper_session_prepare(h,width,height,seed);if(!s)return NULL;
    SniperOnlineSession*o=calloc(1,sizeof(*o));if(!o){sniper_session_destroy(s);return NULL;}
    s->online=o;o->session=s;o->client=c;o->previous_event=c->host.event;o->previous_event_user=c->host.event_user?c->host.event_user:c->host.user;
    for(size_t i=0;i<sniper_level_record_count(bytes);i++){SniperTile t=sniper_tile_decode(level+4*i);if(sniper_map_place(s->map,sizes,t.x,t.y,t.tile)){sniper_session_destroy(s);return NULL;}}
    s->ships.offline_backend=0;s->ships.detach=world_detach;s->ships.movement.local_id=-1;
    o->registry=(SniperPlayerRegistry){s->ships.players,s->ordered,NULL,0,0,1,&s->ships.attachment_hud,&o->defaults,0,allocate_ship,release_ship,o,clock_ms,o};
    o->team=(SniperPlayerTeam){s->ships.players,s->ordered,NULL,0,0,1,1,detach,spawn,o};
    o->death=(SniperDeathWorld){s->ships.players,NULL,-1,0,0,&s->pickups.collection,&s->ships.attachment_hud,allocate_pickup,damage,detach,o};
    o->position=(SniperNetworkPosition){s->ships.players,&s->ships.movement,clock_ms,weapon,NULL,o};
    o->backend=(SniperNetworkBackend){&o->registry,&o->team,&o->death,&s->pickups.collection,&s->ships.attachment_hud,&o->position,&c->game.player_id,s->chat_lines,s->chat_colors,&s->chat_rows,&s->frame.width,&s->chat.dirty,allocate_pickup,clock_ms,extension,o};
    o->gameplay=(SniperOnlineGameplay){&s->combat,&s->pickups,c,-1};sniper_online_gameplay_pickups_bind(&o->gameplay);
    s->binding.backend_override=backend;s->binding.team_override=team;s->binding.detach_override=detach;s->binding.simulate_override=simulate;s->binding.backend_user=o;
    c->host.event=event;c->host.event_user=o;settings(o);
    if(!sniper_network_client_join(c,ship,frequency,audio,width,height)){sniper_session_destroy(s);return NULL;}
    return s;
}
int sniper_online_session_poll(SniperOfflineSession*s){
    if(!s||!s->online)return -1;
    SniperOnlineSession*o=s->online;
    if(o->failed||sniper_network_client_poll(o->client)<0){o->failed=1;return -1;}
    settings(o);s->ships.player_count=o->registry.count;
    if(!o->ready&&o->client->game.ready){int id=o->client->game.player_id;if(id<0||id>=255||!s->ships.players[id]){o->failed=1;return -1;}
        SniperShipObject*local=s->ships.players[id];o->gameplay.local_id=id;s->ships.movement.local_id=id;o->registry.local=local;
        s->start.local=local;s->start.local_id=id;s->start.frequency=local->word[0x68/4];s->start.selected=0;s->start.list_dirty=1;s->start.watched=-1;
        sniper_player_sort(s->ordered,s->ships.player_count,local);
        if(local->word[0x40/4]<4)spawn(o,local,1,1);
        else{ s->chat.enabled=0;for(int i=0;i<s->ships.player_count;i++)if(s->ordered[i]->word[0x40/4]<4){s->start.watched=s->ordered[i]->word[0x44/4];break;}}
        s->ships.movement.watched_id=s->start.watched;if(s->start.history_size>s->frame.height/24)s->start.history_size=s->frame.height/24;
        s->start.cursor_tick=clock_ms(o)/10-10000;s->start.cursor(s->start.user,0,0);
        sniper_session_bind_local(s,local,id);o->ready=1;
    }
    if(o->ready){s->frame.watched=s->start.watched>=0&&s->start.watched<255?s->ships.players[s->start.watched]:NULL;sniper_online_gameplay_sync_pickups(&o->gameplay);}
    return o->ready;
}
void sniper_online_session_release(SniperOfflineSession*s){if(s&&s->online){SniperOnlineSession*o=s->online;if(o->client&&o->client->join_sent){SniperGamePacket bye={0};bye.data[0]=2;bye.length=bye.reliable=bye.copy=1;(void)sniper_network_client_send(o->client,&bye);o->client->join_sent=0;o->client->game.ready=0;}if(o->client&&o->client->host.event_user==o){o->client->host.event=o->previous_event;o->client->host.event_user=o->previous_event_user;}free(o);s->online=NULL;}}
