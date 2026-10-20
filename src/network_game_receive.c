#include "network_game_receive.h"
#include <string.h>
#include <stdio.h>
static uint32_t u16(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8);}
static int32_t s16(const uint8_t*p){uint32_t x=u16(p);return x<32768?(int32_t)x:(int32_t)x-65536;}
static int32_t s8(uint8_t x){return x<128?x:(int32_t)x-256;}
static uint32_t u32(const uint8_t*p){return u16(p)|(u16(p+2)<<16);}
static int32_t signed32(uint32_t x){return x<=INT32_MAX?(int32_t)x:-1-(int32_t)(UINT32_MAX-x);}
static void emit(const SniperGameReceiveHost*h,SniperGameEvent*e){if(h&&h->event)h->event(h->user,e);}
int sniper_game_receive_packet(SniperGameReceive*s,const SniperGameReceiveHost*h,const uint8_t*p,size_t n){
 static const uint16_t minimum[19]={1,2,1,85,2,20,6,5,8,10,1,1,1,1,2,9,5,289,17};
 if(!s||!p||!n)return 0;
 unsigned op=p[0];if(op<1||op>18)return 1;
 if(op>=4&&op<=16&&op!=10&&op!=11&&op!=12&&op!=13&&!s->ready)return 1;
 if(n<minimum[op])return 0;
 SniperGameEvent e={0};int32_t *a=e.args;
 switch(op){
 case 1:s->player_id=p[1];return 1;
 case 2:s->ready=1;return 1;
 case 3:
  e.kind=SNIPER_NET_JOIN;e.count=10;a[0]=p[1];a[3]=s8(p[2]);
  a[4]=s16(p+71);a[5]=s16(p+73);a[6]=signed32(u32(p+75));a[7]=signed32(u32(p+67));a[8]=s16(p+79);a[9]=signed32(u32(p+81));
  e.data=p+3;e.size=32;e.data2=p+35;e.size2=32;break;
 case 4:e.kind=SNIPER_NET_LEAVE;e.count=1;a[0]=p[1];break;
 case 5:{
  uint32_t ms=h&&h->clock_ms?h->clock_ms(h->user):0;
  uint32_t delta=(ms/10+s->clock_offset)%UINT32_C(0x19999999)-u32(p+11);
  if(delta&UINT32_C(0x80000000))delta=0-delta;
  int32_t age=signed32(delta);if(age>80)age=80;
  uint32_t w=u16(p+18);e.kind=SNIPER_NET_POSITION;e.count=15;
  a[0]=age;a[1]=p[10];a[2]=s16(p+2);a[3]=s16(p+4);a[4]=s16(p+6);a[5]=s16(p+8);a[6]=s8(p[1]);
  a[7]=w&7;a[8]=(w>>3)&3;a[9]=(w>>5)&7;a[10]=(w>>8)&3;a[11]=(w>>10)&31;a[12]=w>>15;a[13]=s8(p[15]);a[14]=s16(p+16);break;
 }
 case 6:e.kind=SNIPER_NET_DEATH;e.count=4;a[0]=p[1];a[1]=p[2];a[2]=s16(p+3);a[3]=s8(p[5]);break;
 case 7:{const uint8_t*z=memchr(p+4,0,n-4);if(!z)return 0;e.kind=SNIPER_NET_CHAT;e.count=3;a[1]=p[1];a[2]=s16(p+2);e.data=p+4;e.size=(size_t)(z-(p+4))+1;break;}
 case 8:e.kind=SNIPER_NET_PICKUP;e.count=4;a[0]=s16(p+1);a[1]=s16(p+3);a[2]=p[7];a[3]=s16(p+5);break;
 case 9:e.kind=SNIPER_NET_PICKUP_SPAWN;e.count=4;a[0]=s16(p+2);a[1]=s16(p+4);a[2]=s8(p[1]);a[3]=signed32(u32(p+6));break;
 case 10:case 11:case 12:case 13:return 1;
 case 14:if(n>INT32_MAX)return 0;e.kind=SNIPER_NET_DATA;e.count=3;a[0]=s8(p[1]);a[2]=(int32_t)n-2;e.data=p+2;e.size=n-2;break;
 case 15:e.kind=SNIPER_NET_TEAM;e.count=2;a[0]=signed32(u32(p+1));a[1]=signed32(u32(p+5));break;
 case 16:e.kind=SNIPER_NET_ATTACH;e.count=2;a[0]=s16(p+1);a[1]=s16(p+3);break;
 case 17:memcpy(s->settings,p,289);return 1;
 case 18:{
  /* Original trusts a16-byte filename field and writes arbitrary supplied path.
   * The host decides where to materialize this event; decoding itself has no I/O. */
  const uint8_t*z=memchr(p+1,0,16);if(!z||n>INT32_MAX)return 0;
  e.kind=SNIPER_NET_FILE_OPEN;e.count=2;e.data=p+1;e.size=(size_t)(z-(p+1))+1;e.data2=(const uint8_t*)"wb";e.size2=3;emit(h,&e);
  e=(SniperGameEvent){0};e.kind=SNIPER_NET_FILE_WRITE;e.count=4;e.data=p+17;e.size=n-17;e.args[1]=(int32_t)n-17;e.args[2]=1;emit(h,&e);
  e=(SniperGameEvent){0};e.kind=SNIPER_NET_FILE_CLOSE;e.count=1;emit(h,&e);
  char message[32];snprintf(message,sizeof(message),"File received: %s",(const char*)p+1);
  e=(SniperGameEvent){0};e.kind=SNIPER_NET_CHAT;e.count=3;e.args[1]=s->player_id;e.args[2]=s->player_id;e.data=(const uint8_t*)message;e.size=strlen(message)+1;emit(h,&e);return 1;
 }
 default:return 1;
 }
 emit(h,&e);return 1;
}
void sniper_game_receive_drain(SniperGameReceive*s,const SniperGameReceiveHost*h){
 if(!s||!h||!h->next)return;
 size_t n;const uint8_t*p;while((p=h->next(h->user,&n))!=NULL)sniper_game_receive_packet(s,h,p,n);
}
