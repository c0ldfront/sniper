#include "network_backend.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static SniperShipObject *players[1024],*ordered[1024];
static int released,awards,damages,detaches,spawns,extensions;
static SniperShipObject*alloc_ship(void*u){(void)u;return calloc(1,sizeof(SniperShipObject));}
static void free_ship(void*u,SniperShipObject*s){(void)u;free(s);released++;}
static SniperPickupObject*alloc_pickup(void*u){(void)u;return calloc(1,sizeof(SniperPickupObject));}
static void free_pickup(void*u,SniperPickupObject*p){(void)u;free(p);}
static SniperShipObject*lookup(void*u,int32_t id){(void)u;return players[id];}
static void award(void*u,SniperShipObject*s,int32_t code){(void)u;assert(s==players[42]&&code==5);awards++;}
static void damage(void*u,SniperShipObject*s,int32_t amount,int32_t owner){(void)u;assert(s==players[7]&&amount==-1&&owner==42);damages++;}
static void detach(void*u,int32_t id){(void)u;assert(id==-1);detaches++;}
static void spawn(void*u,SniperShipObject*s,int reset,int move){(void)u;assert(s==players[42]&&reset==1&&move==0);spawns++;}
static uint32_t clock_ms(void*u){(void)u;return 123456;}
static void extension(void*u,const SniperGameEvent*e){(void)u;assert(e->kind==SNIPER_NET_DATA||e->kind==SNIPER_NET_FILE_OPEN||e->kind==SNIPER_NET_FILE_WRITE||e->kind==SNIPER_NET_FILE_CLOSE);extensions++;}
static void word(uint8_t*p,unsigned x){p[0]=x;p[1]=x>>8;}
static void dword(uint8_t*p,unsigned x){word(p,x);word(p+2,x>>16);}
int main(void){
 uint8_t*map=calloc(1,1024*1024);assert(map);SniperNotices notices={0};SniperShipDefaults defaults={0};defaults.energy_maximum=1000000;
 SniperPlayerRegistry registry={players,ordered,NULL,0,0,0,&notices,&defaults,0,alloc_ship,free_ship,NULL,NULL,NULL};
 SniperPickupObject*items[16]={0};SniperPickupWorld pickups={items,0,map,NULL,0,0,lookup,free_pickup,award,NULL};
 SniperDeathWorld death={players,NULL,42,2,500,&pickups,&notices,alloc_pickup,damage,detach,NULL};
 SniperPlayerTeam team={players,ordered,NULL,0,0,0,1,detach,spawn,NULL};
 SniperMoveWorld movement={0};movement.map=map;movement.bounce_divisor=2;movement.local_id=42;movement.watched_id=-1;
 SniperNetworkPosition position={players,&movement,clock_ms,NULL,NULL,NULL};
 int32_t local_id=42,rows=8,width=640,dirty=0;char lines[8][128]={{0}};int32_t colors[8]={0};
 SniperNetworkBackend backend={&registry,&team,&death,&pickups,&notices,&position,&local_id,lines,colors,&rows,&width,&dirty,alloc_pickup,clock_ms,extension,NULL};
 SniperGameReceive state={0};SniperGameReceiveHost host={&backend,clock_ms,sniper_network_backend_event,NULL};
 uint8_t id[]={1,42},ready[]={2};assert(sniper_game_receive_packet(&state,&host,id,2));assert(state.player_id==42);
 uint8_t join[85]={3};join[1]=42;memcpy(join+3,"Local",6);memcpy(join+35,"Squad",6);dword(join+67,3);word(join+79,65535);
 assert(sniper_game_receive_packet(&state,&host,join,sizeof(join)));assert(registry.count==1&&players[42]&&!registry.local);
 registry.local=players[42];players[42]->word[0x48/4]=1000000;players[42]->word[0x4c/4]=1000000;
 join[1]=7;memcpy(join+3,"Remote",7);assert(sniper_game_receive_packet(&state,&host,join,sizeof(join)));assert(registry.count==2&&registry.dirty);
 assert(sniper_game_receive_packet(&state,&host,ready,1));
 uint8_t pos[20]={5};pos[1]=10;word(pos+2,2000);word(pos+4,2000);pos[10]=7;dword(pos+11,12345);pos[15]=31;word(pos+16,40);
 assert(sniper_game_receive_packet(&state,&host,pos,20));assert(players[7]->word[0x48/4]==2000000&&players[7]->word[0x4c/4]==2000000);assert(players[7]->word[0x58/4]==10000&&players[7]->word[0xa0/4]==40&&players[7]->word[0xbc/4]==48);
 uint8_t prize[10]={9,5};word(prize+2,50);word(prize+4,60);dword(prize+6,500);assert(sniper_game_receive_packet(&state,&host,prize,10));assert(pickups.count==1&&map[60*1024+50]==255);
 uint8_t collect[8]={8};word(collect+1,50);word(collect+3,60);word(collect+5,5);collect[7]=42;assert(sniper_game_receive_packet(&state,&host,collect,8));assert(pickups.count==0&&map[60*1024+50]==0&&awards==1);
 uint8_t chat[]={7,7,255,255,'h','e','l','l','o',0};assert(sniper_game_receive_packet(&state,&host,chat,sizeof(chat)));assert(strstr(lines[0],"hello")&&strstr(lines[0],"Remote")&&dirty);
 uint8_t attach[5]={16};word(attach+1,7);word(attach+3,42);assert(sniper_game_receive_packet(&state,&host,attach,5));assert(players[7]->word[0x6c/4]==42);
 uint8_t frequency[9]={15};dword(frequency+1,42);dword(frequency+5,99);assert(sniper_game_receive_packet(&state,&host,frequency,9));assert(players[42]->word[0x68/4]==99&&players[42]->word[0x5c/4]==0&&spawns==1);
 uint8_t killed[]={6,42,7,25,0,255};assert(sniper_game_receive_packet(&state,&host,killed,6));assert(damages==1&&players[42]->word[0x78/4]==1&&players[7]->word[0x7c/4]==1);
 uint8_t data[]={14,7,1,2,3};assert(sniper_game_receive_packet(&state,&host,data,5));uint8_t file[18]={18};memcpy(file+1,"map.lvl",8);file[17]=99;assert(sniper_game_receive_packet(&state,&host,file,18));assert(extensions==4&&strstr(lines[0],"File received: map.lvl"));
 uint8_t leave[]={4,7};assert(sniper_game_receive_packet(&state,&host,leave,2));assert(!players[7]&&registry.count==1&&released==1);
 sniper_player_remove(&registry,42);free(map);puts("PASS decoded online backend flow: join, ready, actual movement, pickup spawn/collect, chat, attachment, team reset, death, file/data extension and leave");
}
