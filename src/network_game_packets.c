#include "network_game_packets.h"
#include <string.h>
static void word(uint8_t *p,uint32_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void dword(uint8_t *p,uint32_t x){word(p,x);word(p+2,x>>16);}
static void init(SniperGamePacket *p,uint8_t type,uint32_t n,uint32_t reliable,uint32_t copy){p->data[0]=type;p->length=n;p->reliable=reliable;p->copy=copy;}
void sniper_packet_position(SniperGamePacket *p,const int32_t a[13],uint32_t ms,uint32_t offset){
 init(p,3,19,0,1);p->data[1]=(uint8_t)a[4];
 for(int i=0;i<4;i++)word(p->data+2+2*i,(uint32_t)a[i]);
 dword(p->data+10,(ms/10+offset)%UINT32_C(0x19999999));
 p->data[14]=(uint8_t)a[11];word(p->data+15,(uint32_t)a[12]);
 word(p->data+17,((uint32_t)a[5]&7)|(((uint32_t)a[6]&3)<<3)|(((uint32_t)a[7]&7)<<5)|(((uint32_t)a[8]&3)<<8)|(((uint32_t)a[9]&31)<<10)|(((uint32_t)a[10]&1)<<15));
}
void sniper_packet_death(SniperGamePacket *p,int32_t kind,int32_t player){init(p,5,4,1,1);p->data[1]=(uint8_t)kind;word(p->data+2,(uint32_t)player);}
void sniper_packet_pickup(SniperGamePacket *p,int32_t x,int32_t y,int32_t prize,int32_t player,uint32_t reliable){init(p,7,8,reliable,1);word(p->data+1,(uint32_t)x);word(p->data+3,(uint32_t)y);word(p->data+5,(uint32_t)prize);p->data[7]=(uint8_t)player;}
int sniper_packet_chat(SniperGamePacket *p,int32_t recipient,const char *text,uint32_t reliable){size_t n=0;if(!text)return 0;while(n<509&&text[n])n++;if(n==509)return 0;init(p,6,(uint32_t)n+4,recipient>=0?1:reliable,1);word(p->data+1,(uint32_t)recipient);memcpy(p->data+3,text,n+1);return 1;}
void sniper_packet_ship(SniperGamePacket *p,int32_t ship){init(p,8,2,1,1);p->data[1]=(uint8_t)ship;}
void sniper_packet_team(SniperGamePacket *p,int32_t team){init(p,16,5,1,1);dword(p->data+1,(uint32_t)team);}
void sniper_packet_attach(SniperGamePacket *p,int32_t player){init(p,17,3,1,1);word(p->data+1,(uint32_t)player);}
void sniper_packet_release_children(SniperGamePacket p[2],int32_t team){sniper_packet_team(p,team);dword(p->data+1,(uint32_t)team+1000);sniper_packet_team(p+1,team);}
size_t sniper_packet_cached_into(uint8_t *out,size_t capacity,uint32_t *copy,int32_t recipient,int32_t slot,const uint8_t *data,size_t n,int *cached){
 if(!data||!cached||!out||!copy)return 0;
 if(!*cached&&n>UINT32_MAX-4)return 0;
 size_t total=*cached?4:n+4;if(total>capacity)return 0;
 out[0]=15;word(out+1,(uint32_t)recipient);out[3]=(uint8_t)slot;
 *copy=*cached?1:0;if(!*cached)memcpy(out+4,data,n);*cached=1;return total;
}
int sniper_packet_cached(SniperGamePacket *p,int32_t recipient,int32_t slot,const uint8_t *data,size_t n,int *cached){
 size_t total=sniper_packet_cached_into(p->data,sizeof(p->data),&p->copy,recipient,slot,data,n,cached);
 if(!total)return 0;
 p->length=(uint32_t)total;p->reliable=1;return 1;
}
