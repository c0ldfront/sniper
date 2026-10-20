#include "network_login.h"
#include <string.h>
#include <stdlib.h>
static uint32_t rd(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void wr(uint8_t*p,uint32_t v){for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void start(SniperGamePacket*p,uint8_t type,uint32_t n){memset(p,0,sizeof(*p));p->data[0]=type;p->length=n;p->reliable=p->copy=1;}
int sniper_login_packet(SniperGamePacket*p,const char*name,const char*pass,int create){if(!p||!name||!pass)return 0;size_t n=strlen(name),m=strlen(pass);if(n>31||m>31)return 0;start(p,9,98);p->data[1]=create!=0;memcpy(p->data+2,name,n+1);memcpy(p->data+34,pass,m+1);return 1;}
int sniper_login_reply(SniperLoginReply*s,const uint8_t*p,size_t n){if(!s||!p||!n)return -1;if(p[0]!=10)return 0;if(n<18)return -1;s->status=p[1];s->version=(int32_t)rd(p+2);s->map_checksum=rd(p+6);s->exe_checksum=rd(p+10);s->arena_checksum=rd(p+14);return 1;}
int sniper_login_accepted(const SniperLoginReply*s){return s->status!=1&&s->status!=2&&s->status!=3;}
int sniper_login_version_matches(const SniperLoginReply*s,uint32_t checksum){return s->version<=87&&s->exe_checksum==checksum;}
void sniper_login_join_packet(SniperGamePacket*p,int32_t ship,int32_t param,int32_t audio,int32_t width,int32_t height){start(p,1,18);p->data[1]=(uint8_t)ship;wr(p->data+2,(uint32_t)param);wr(p->data+6,(uint32_t)audio);wr(p->data+10,(uint32_t)width);wr(p->data+14,(uint32_t)height);}
void sniper_login_map_request(SniperGamePacket*p){start(p,12,1);}
void sniper_login_arena_request(SniperGamePacket*p){start(p,18,1);}
void sniper_login_cancel_download(SniperGamePacket*p){start(p,14,1);}
int sniper_login_map_name(char out[13],const char*host,const char*port){if(!out||!host||!port)return 0;size_t h=strlen(host),p=strlen(port);if(h>SIZE_MAX-p-5)return 0;char*b=malloc(h+p+5);if(!b)return 0;size_t n=0;for(size_t i=0;i<h;i++)if(host[i]!='.')b[n++]=host[i];for(size_t i=0;i<p;i++)if(port[i]!='.')b[n++]=port[i];memcpy(b+n,".lvl",5);n+=4;size_t at=n>12?n-12:0;memcpy(out,b+at,n-at+1);free(b);return 1;}
int sniper_login_download(SniperLoginDownload*s,const uint8_t*p,size_t n,void(*write_at)(void*,int32_t,const uint8_t*,size_t),void*u){if(!s||!p||!n)return -1;if(p[0]!=12&&p[0]!=13)return 0;if(n<5)return -1;int32_t value=(int32_t)rd(p+1);if(p[0]==13)s->total=value;else if(value==-1)s->done=1;else{s->offset=value;if(write_at)write_at(u,value,p+5,n-5);}return 1;}
