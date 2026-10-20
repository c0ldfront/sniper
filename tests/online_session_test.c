#include "online_session.h"
#include "network_compression.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint8_t data[512];int n;} Packet;
typedef struct {Packet queue[1024];unsigned head,count,sequence;uint32_t clock;int controls,positions,deaths;SniperLocalConfig config;} Peer;
static void le16(uint8_t*p,unsigned n){p[0]=n;p[1]=n>>8;}
static void le32(uint8_t*p,unsigned n){le16(p,n);le16(p+2,n>>16);}
static void raw(Peer*p,const uint8_t*b,int n){assert(p->count<1024&&n<=512);Packet*q=&p->queue[(p->head+p->count++)%1024];memcpy(q->data,b,n);q->n=n;}
static void reliable(Peer*p,const uint8_t*b,int n){uint8_t q[512]={0,2};le32(q+2,p->sequence++);le16(q+6,(p->sequence-1001)%256);memcpy(q+8,b,n);raw(p,q,n+8);}
static uint32_t clock_ms(void*u){return ((Peer*)u)->clock;}
static int32_t key(void*u,int32_t vk){Peer*p=u;return p->controls&&(vk==38||vk==17)?0x8000:0;}
static int32_t send_packet(void*u,const uint8_t*b,int32_t n){Peer*p=u;if(n>=2&&!b[0]){
 if(b[1]==0){uint8_t reply[10]={0,1};memcpy(reply+2,b+2,4);le32(reply+6,p->clock/10);raw(p,reply,10);return n;}
 if(b[1]==3)return n;
 if(b[1]==2){uint8_t ack[8]={0,3};memcpy(ack+2,b+2,6);raw(p,ack,8);b+=8;n-=8;}else return n;
 }
 if(b[0]==1){uint8_t cfg[289]={17};memcpy(cfg+1,&p->config,288);reliable(p,cfg,289);uint8_t id[2]={1,42};reliable(p,id,2);
 uint8_t join[85]={3,42};memcpy(join+3,"Local42",8);le32(join+67,17);le16(join+79,65535);reliable(p,join,85);
 join[1]=7;memcpy(join+3,"Remote7",8);reliable(p,join,85);uint8_t ready=2;reliable(p,&ready,1);
 }else if(b[0]==3)p->positions++;else if(b[0]==5)p->deaths++;
 return n+(b[0]!=3?8:0); /* reliable send return ignored,position exact below */
}
static int32_t read_packet(void*u,uint8_t*b,int32_t n,int peek){Peer*p=u;if(!p->count)return -1;Packet*q=&p->queue[p->head];assert(n>=q->n);memcpy(b,q->data,q->n);int z=q->n;if(!peek){p->head=(p->head+1)%1024;p->count--;}return z;}
static void cursor(void*u,int32_t x,int32_t y){(void)u;(void)x;(void)y;}
static void post(void*u){(void)u;}
static uint8_t *decode(void*u,const uint8_t*p,size_t n,size_t*out){(void)u;int32_t z;uint8_t*r=sniper_network_decompress(p,(int32_t)n,&z);*out=r?(size_t)z:0;return r;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint8_t*level=malloc((size_t)n);assert(level&&fread(level,1,(size_t)n,f)==(size_t)n);fclose(f);
 Peer*p=calloc(1,sizeof(*p));assert(p);p->clock=100000;p->sequence=1000;sniper_local_config(&p->config);
 SniperNetworkClient client;SniperNetworkClientHost host={.transport={p,clock_ms,read_packet,send_packet,NULL,sniper_network_compress_callback,decode}};sniper_network_client_init(&client,&host);
 SniperSessionHost sh={.clock_ms=clock_ms,.key_state=key,.post_command=post,.cursor=cursor,.user=p};SniperTileSize sizes[256]={{0}};for(int i=216;i<220;i++)sizes[i]=(SniperTileSize){16,16};
 SniperOfflineSession*s=sniper_online_session_begin(&sh,&client,level,(size_t)n,sizes,0,17,1,640,480,1234);assert(s);assert(sniper_online_session_poll(s)==1);assert(s->start.local_id==42&&s->ships.players[42]&&s->ships.players[7]&&!s->ships.players[0]);
 assert(s->ships.player_count==2&&s->frame.local==s->ships.players[42]);s->start.local->word[0x48/4]=2000000;s->start.local->word[0x4c/4]=2000000;p->controls=1;
 for(int i=0;i<1800;i++){p->clock+=20;if(i==300)s->lifecycle.key=123;if(i==700)s->active=0;if(i==760)s->active=1;assert(sniper_session_step(s));assert(!s->ships.players[0]&&s->frame.local==s->ships.players[42]);}
 assert(p->positions>20&&p->deaths==1);sniper_session_destroy(s);sniper_network_client_clear(&client);free(p);free(level);puts("PASS production online session1800 frames: server settings/join,localID42 without slot0,real controls/fire/death,remote registry,inactive simulation and cleanup");
}
