#include "network_client.h"
#include "network_backend.h"
#include "local_config.h"
#include "ship_spawn.h"
#include "offline_session.h"
#include "online_gameplay.h"
#include <stdlib.h>
#include <string.h>
static SniperNetworkClient client;
static SniperShipObject ships[2],*players[1024],*ordered[2];
static SniperMoveWorld movement;
static SniperPlayerRegistry registry;
static SniperNetworkPosition position;
static SniperNetworkBackend backend;
static SniperPlayerTeam team;
static SniperLocalConfig config;
static SniperShipDefaults defaults;
static uint32_t random_state;static int32_t position_dirty;
static void (*weapon_trace)(const int32_t *,const void *);
static uint32_t (*clock_callback)(void *);
static SniperOfflineSession *session;
static SniperOnlineGameplay gameplay;
static SniperProjectile pool[1024];static int allocated,with_projectiles;
static int32_t no_key(void*u,int32_t key){(void)u;(void)key;return 0;}
static void effect(int kind,int a,int b,int c){int32_t args[14]={kind,a,b,c};weapon_trace(args,NULL);}
static SniperProjectile*allocate(void*u){(void)u;if(allocated>=1024)abort();effect(1,allocated,0,0);return &pool[allocated++];}
static void release(void*u,SniperProjectile*p){(void)u;effect(2,(int)(p-pool),0,0);}
static void sound(void*u,int32_t h,int32_t x,int32_t y){(void)u;effect(3,h,x,y);}
static void audio(void*u,int32_t h,const SniperAudioRequest*r){(void)u;effect(4,h,r->volume,r->pan);effect(5,r->loop,r->pitch,0);}

static void weapon(void*u,const int32_t*a){(void)u;weapon_trace(a,players[a[12]]);if(with_projectiles)sniper_online_gameplay_weapon(&gameplay,a);}
static uint32_t clock_ms(void*u){(void)u;return clock_callback(NULL);}
static void detach(void*u,int32_t target){(void)u;players[42]->word[0x6c/4]=target;}
static void spawn(void*u,SniperShipObject*p,int reset,int move){(void)u;sniper_ship_spawn(p->word,&defaults,movement.map,2,clock_ms(NULL),&random_state,reset,move,&position_dirty);}
void critical_network_init(uint32_t(*clock)(void*),int32_t(*read)(void*,uint8_t*,int32_t,int),int32_t(*send)(void*,const uint8_t*,int32_t),void(*trace)(const int32_t*,const void*),const void*initial,const uint8_t*map,uint32_t offset,int compose_team){
 with_projectiles=compose_team==2;compose_team=compose_team==1;
 memset(&client,0,sizeof client);memset(players,0,sizeof players);memcpy(ships,initial,sizeof ships);players[7]=&ships[0];players[42]=&ships[1];ordered[0]=players[7];ordered[1]=players[42];clock_callback=clock;weapon_trace=trace;
 movement=(SniperMoveWorld){0};movement.map=(uint8_t*)map;movement.local_id=42;movement.watched_id=-1;movement.bounce_divisor=16;
 sniper_local_config(&config);config.word[(0x4282ad-0x4281c9)/4]=1;sniper_local_ship_defaults(&config,&defaults);random_state=12345;position_dirty=-7;
 registry=(SniperPlayerRegistry){0};registry.players=players;registry.ordered=ordered;registry.local=players[42];registry.count=2;
 position=(SniperNetworkPosition){players,&movement,clock_ms,weapon,NULL,NULL};backend=(SniperNetworkBackend){0};backend.registry=&registry;backend.position=&position;
 team=(SniperPlayerTeam){players,ordered,players[42],2,0,0,1,detach,spawn,NULL};if(compose_team)backend.team=&team;
 SniperNetworkClientHost host={0};host.transport.clock_ms=clock;host.transport.read=read;host.transport.send=send;host.event=sniper_network_backend_event;host.event_user=&backend;
 if(with_projectiles){
  SniperSessionHost sh={.clock_ms=clock,.key_state=no_key,.sound=sound,.audio=audio};session=sniper_session_prepare(&sh,640,480,12345);if(!session)abort();session->start.local_id=42;
  memcpy(&session->ships.config,&config,sizeof config);memcpy(session->map,map,1024*1024);session->ships.players[7]=players[7];session->ships.players[42]=players[42];session->ordered[0]=players[7];session->ordered[1]=players[42];session->ships.player_count=2;session->ships.random_state=12345;session->ships.movement.local_id=42;
  session->combat.allocate=allocate;session->combat.release=release;session->combat.next_group=123;session->combat.ordered_ships=session->ordered;allocated=0;memset(pool,0x55,sizeof pool);gameplay=(SniperOnlineGameplay){&session->combat,&session->pickups,&client,42};
 }
 sniper_network_client_init(&client,&host);client.game.ready=1;client.game.player_id=42;client.transport.receiver.clock_offset=(int32_t)offset;
}
int critical_network_poll(void){return sniper_network_client_poll(&client);}
const void*critical_network_ships(void){return ships;}
void critical_network_state(uint32_t*out){out[0]=client.transport.receiver.expected;out[1]=client.transport.last_receive_tick;out[2]=client.transport.bytes_received;out[3]=client.transport.packets_received;out[4]=client.transport.sender.bytes_sent;out[5]=client.transport.sender.packets_sent;}
void critical_network_clear(void){sniper_network_client_clear(&client);if(session){sniper_voice_recordings_close(&session->voice_recordings);free(session);session=NULL;}}

const void*critical_network_config(void){return &config;}
void critical_network_team_state(int32_t*out){out[0]=ordered[0]->word[0x44/4];out[1]=ordered[1]->word[0x44/4];out[2]=registry.selected;out[3]=registry.dirty;out[4]=(int32_t)random_state;out[5]=position_dirty;}

void critical_network_projectiles(int which,void*out){
 if(which==0)memcpy(out,pool,sizeof pool);
 if(which==1){int32_t state[]={allocated,session->combat.count,session->combat.next_group,(int32_t)session->ships.random_state,session->combat.repel_timer};memcpy(out,state,sizeof state);}
 if(which==2)for(int i=0;i<512;i++)((int32_t*)out)[i]=session->combat.projectiles[i]?(int32_t)(session->combat.projectiles[i]-pool):-1;
}
unsigned critical_network_numeric(void){unsigned short control=0x027f;__asm__ volatile("fldcw %0"::"m"(control));__asm__ volatile("fnstcw %0":"=m"(control));return control;}
