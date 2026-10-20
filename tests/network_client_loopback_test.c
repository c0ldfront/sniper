#define _POSIX_C_SOURCE 200809L
#include "../src/network_client.h"
#include "../src/network_lifecycle.h"
#include "../src/network_backend.h"
#include "../src/network_compression.h"
#include "../platform/network_socket.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct {SniperNetworkBackend *backend;uint8_t file[128];size_t written;int done,joins,chats,teams,attaches,pickups,positions,data;} Events;
static void event(void*u,const SniperGameEvent*e){Events*s=u;sniper_network_backend_event(s->backend,e);switch(e->kind){case SNIPER_NET_JOIN:assert(e->args[0]==7);s->joins++;break;case SNIPER_NET_CHAT:s->chats++;break;case SNIPER_NET_TEAM:s->teams++;break;case SNIPER_NET_ATTACH:s->attaches++;break;case SNIPER_NET_PICKUP:s->pickups++;break;case SNIPER_NET_POSITION:s->positions++;break;case SNIPER_NET_DATA:s->data++;break;default:break;}}
static SniperShipObject *players[255],*ordered[256];
static SniperShipObject*alloc_ship(void*u){(void)u;return calloc(1,sizeof(SniperShipObject));}
static void release_ship(void*u,SniperShipObject*s){(void)u;free(s);}
static void detach(void*u,int32_t id){(void)u;(void)id;}
static void spawn(void*u,SniperShipObject*s,int reset,int move){(void)u;(void)s;(void)reset;(void)move;}
static void write_at(void*u,int32_t at,const uint8_t*p,size_t n){Events*s=u;assert(at>=0&&(size_t)at+n<=sizeof(s->file));memcpy(s->file+at,p,n);if((size_t)at+n>s->written)s->written=(size_t)at+n;}
static void done(void*u,const SniperLoginDownload*d){Events*s=u;assert(d->done&&d->total==(int32_t)s->written);s->done++;}
static uint8_t *decode(void*u,const uint8_t*p,size_t n,size_t*out){(void)u;int32_t z;assert(n<INT32_MAX);uint8_t*r=sniper_network_decompress(p,(int32_t)n,&z);*out=r?(size_t)z:0;return r;}
static void wait_one(void*u){(void)u;struct timespec t={0,1000000};nanosleep(&t,NULL);}
static void pump(SniperNetworkClient*c){assert(sniper_network_client_poll(c)>=0);assert(sniper_network_transport_alive(&c->transport,1));wait_one(NULL);}
int main(int argc,char**argv){assert(argc==2);SniperNetworkSocket socket;assert(sniper_network_socket_open(&socket,"127.0.0.1",(uint16_t)atoi(argv[1])));Events e={0};SniperNetworkClient c;
    uint8_t *map=calloc(1,1024*1024);assert(map);SniperNotices notices={0};SniperShipDefaults defaults={0};defaults.energy_maximum=1000000;
    SniperPlayerRegistry registry={players,ordered,NULL,0,0,0,&notices,&defaults,0,alloc_ship,release_ship,NULL,NULL,NULL};
    SniperPlayerTeam team={players,ordered,NULL,0,0,0,1,detach,spawn,NULL};
    SniperMoveWorld movement={0};movement.map=map;movement.bounce_divisor=2;movement.local_id=7;movement.watched_id=-1;
    SniperNetworkPosition position={players,&movement,sniper_network_socket_clock,NULL,NULL,NULL};
    int32_t rows=8,width=640,dirty=0;char lines[8][128]={{0}};int32_t colors[8]={0};
    SniperNetworkBackend backend={&registry,&team,NULL,NULL,&notices,&position,&c.game.player_id,lines,colors,&rows,&width,&dirty,NULL,sniper_network_socket_clock,NULL,NULL};e.backend=&backend;
    SniperNetworkClientHost h={{&socket,sniper_network_socket_clock,sniper_network_socket_read,sniper_network_socket_send,sniper_network_socket_close,sniper_network_compress_callback,decode},&e,event,NULL,write_at,done,NULL};sniper_network_client_init(&c,&h);
    uint32_t start=sniper_network_socket_clock(NULL);while(sniper_network_socket_clock(NULL)-start<1500)pump(&c);
    assert(sniper_network_client_login(&c,"Pilot87","fixture-password",0));start=sniper_network_socket_clock(NULL);while(!c.login_valid){assert(sniper_network_socket_clock(NULL)-start<10000);pump(&c);}assert(sniper_login_accepted(&c.login)&&sniper_login_version_matches(&c.login,0x68fc54a0));
    assert(sniper_network_client_request_map(&c));start=sniper_network_socket_clock(NULL);while(!e.done){assert(sniper_network_socket_clock(NULL)-start<10000);pump(&c);}assert(e.written==20&&!memcmp(e.file,"controlled map bytes",20));
    assert(sniper_network_client_join(&c,2,17,1,640,480));start=sniper_network_socket_clock(NULL);while(!c.game.ready){assert(sniper_network_socket_clock(NULL)-start<10000);pump(&c);}assert(c.game.player_id==7&&e.joins==1&&c.game.settings[0]==17&&c.game.settings[288]==0xa5);
    assert(registry.count==1&&players[7]);registry.local=players[7];
    assert(sniper_network_client_chat(&c,-1,"hello peer",1));sniper_network_client_team(&c,22);sniper_network_client_attach(&c,-1);sniper_network_client_pickup(&c,9,10,3,1);
    assert(sniper_network_client_cached(&c,-1,2,(const uint8_t*)"cache",5));assert(sniper_network_client_cached(&c,-1,2,(const uint8_t*)"cache",5));
    int32_t args[13]={0};args[0]=1234;sniper_network_client_position(&c,args);
    start=sniper_network_socket_clock(NULL);while(e.positions<1||c.transport.sender.count){assert(sniper_network_socket_clock(NULL)-start<10000);pump(&c);}assert(e.chats>=1&&e.teams==1&&e.attaches==1&&e.pickups==1&&e.data==2);
    assert(players[7]->word[0x68/4]==22&&dirty&&strstr(lines[0],"hello peer"));
    sniper_network_transport_shutdown(&c.transport,wait_one,NULL);sniper_network_client_clear(&c);assert(!socket.connected);sniper_player_remove(&registry,7);free(map);
    puts("PASS real local game peer: TCP/UDP, clock probes, reliable auth, map download, ID/settings/player/ready, outbound commands/live world mutation, remote position and graceful close");return 0;
}
