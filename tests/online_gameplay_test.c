#include "online_gameplay.h"
#include "offline_session.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t now;int frame,sends,positions,deaths,pickups;} Host;
static SniperOnlineGameplay*g_current;
static uint32_t clock_ms(void*u){return ((Host*)u)->now;}
static int32_t key(void*u,int32_t vk){Host*h=u;return ((vk==38&&h->frame%200<120)||(vk==39&&h->frame%160<50)||(vk==17&&h->frame%5==0))?0x8000:0;}
static int32_t send_packet(void*u,const uint8_t*p,int32_t n){Host*h=u;h->sends++;if(n&&p[0]==3)h->positions++;if(n&&p[0]==7)h->pickups++;if(n>=9&&p[0]==0&&p[1]==2){if(p[8]==5)h->deaths++;if(p[8]==7)h->pickups++;}return n;}
static int32_t read_packet(void*u,uint8_t*p,int32_t n,int peek){(void)u;(void)p;(void)n;(void)peek;return -1;}
static void sound(void*u,int32_t h,int32_t x,int32_t y){(void)u;(void)h;(void)x;(void)y;}
static void detach(void*u,int32_t target){(void)u;sniper_network_client_attach(g_current->client,target);}
static void event(void*u,const SniperGameEvent*e){
 (void)u;SniperOnlineGameplay*g=g_current;
 if(e->kind==SNIPER_NET_PICKUP)sniper_online_gameplay_collect(g,e->args[0],e->args[1],e->args[2],e->args[3]);
 else if(e->kind==SNIPER_NET_ATTACH)sniper_attachment(g->combat->ships->players,g->local_id,&g->combat->ships->attachment_hud,e->args[0],e->args[1]);
 else assert(0);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint8_t*bytes=malloc((size_t)n);assert(bytes&&fread(bytes,1,(size_t)n,f)==(size_t)n);fclose(f);
 for(int pass=0;pass<3;pass++){
 Host h={1000,0,0,0,0,0};SniperTileSize sizes[256]={{0}};for(int i=0;i<4;i++){int side=(int[]){16,32,64,96}[i];sizes[216+i]=(SniperTileSize){side,side};}
 SniperSessionHost host={.clock_ms=clock_ms,.key_state=key,.sound=sound,.user=&h};SniperOfflineSession*s=sniper_session_create(&host,bytes,(size_t)n,sizes,"OnlinePilot",0,1,640,480,87);assert(s);
 int id=(int[]){1,42,254}[pass];SniperShipObject*local=s->ships.players[0];s->ships.players[0]=NULL;s->ships.players[id]=local;local->word[0x44/4]=id;s->ships.movement.local_id=id;s->ships.offline_backend=0;s->ships.detach=detach;
 SniperNetworkClient client;SniperNetworkClientHost nh={0};nh.transport.user=&h;nh.transport.clock_ms=clock_ms;nh.transport.send=send_packet;nh.transport.read=read_packet;nh.user=&h;nh.event=event;sniper_network_client_init(&client,&nh);client.game.player_id=id;client.game.ready=1;client.transport.sender.bundle_threshold=0;
 SniperOnlineGameplay g={&s->combat,&s->pickups,&client,id};g_current=&g;
 /* This fixture borrows an offline-initialized world; restore original sound
  * seam before replacing its already-installed movement hooks. */
 s->ships.movement.impact_sound=s->pickups.impact_sound;s->ships.movement.user=s->pickups.impact_user;sniper_online_gameplay_pickups_bind(&g);
 int shots=0,moved=0;int32_t x=local->word[0x48/4];
 for(h.frame=0;h.frame<600;h.frame++){
  h.now+=10;s->ships.milliseconds=h.now;s->control.ticks=1;s->position.ticks=(int32_t)(h.now/10);s->position.interval=s->ships.config.word[(0x428205-0x4281c9)/4];s->control.speed_exempt=s->combat.repel_timer>0;
  sniper_online_gameplay_control(&g,&s->control);int32_t ticks=1;sniper_online_gameplay_simulation(&g,&ticks);
  if(local->word[0x48/4]!=x)moved=1;
  if(s->combat.count)shots=1;
  assert(!s->ships.players[0]);
 }
 assert(shots&&moved&&h.positions>0&&s->pickups.collection.count==0);
 /* Movement local request must serialize then synchronously collect/award at
  * actual online ID. Prize0 isolates ordering without randomized prize effects. */
 int px=500,py=500;assert(sniper_pickup_create(&s->pickups.collection,px,py,0,100,s->pickups.allocate,s->pickups.owner)==1);sniper_online_gameplay_sync_pickups(&g);assert(s->ships.movement.pickup_count==1);
 s->ships.movement.pickup(s->ships.movement.user,px,py,id,0,1);assert(!s->pickups.collection.count&&!s->ships.movement.pickup_count&&h.pickups==1);
 /* Fatal local damage emits network death, never offline immediate score echo. */
 local->word[0x48/4]=8000000;local->word[0x4c/4]=8000000;s->map[500*1024+500]=0;local->word[0x5c/4]=100;local->word[0x108/4]=0;int oldloss=local->word[0x7c/4];
 sniper_online_gameplay_damage(&g,local,101,7);assert(local->word[0x5c/4]<0&&h.deaths==1&&local->word[0x7c/4]==oldloss);
 /* Direct remote weapon spawn uses real combat allocation/explosion pipeline. */
 int32_t weapon[14]={3,1,1,1,3,0,0,0,8000000,8000000,0,0,7,2};sniper_online_gameplay_weapon(&g,weapon);assert(s->combat.count>0);
 for(int k=0;k<200;k++){h.now+=10;s->ships.milliseconds=h.now;int32_t ticks=1;sniper_online_gameplay_simulation(&g,&ticks);}
 sniper_network_client_clear(&client);
 /* Existing offline destructor owns slot0; put ownership back only for cleanup. */
 s->ships.players[0]=local;s->ships.players[id]=NULL;local->word[0x44/4]=0;sniper_session_destroy(s);
 }
 free(bytes);puts("PASS online gameplay IDs1/42/254: 1800 control/simulation frames, real projectiles, position wire sends, pickup echo, fatal death wire without offline scoring, remote bombs and cleanup");
}
