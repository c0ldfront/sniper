#include "network_client.h"
#include "network_lifecycle.h"
#include <stdlib.h>
#include <string.h>
static uint32_t now(SniperNetworkClient*c){return c->host.transport.clock_ms?c->host.transport.clock_ms(c->host.transport.user):0;}
static uint32_t game_clock(void*u){return now(u);}
static void event(void*u,const SniperGameEvent*e){SniperNetworkClient*c=u;if(c->host.event)c->host.event(c->host.event_user?c->host.event_user:c->host.user,e);}
static void write_at(void*u,int32_t offset,const uint8_t*p,size_t n){SniperNetworkClient*c=u;if(c->host.download_write)c->host.download_write(c->host.user,offset,p,n);}
void sniper_network_client_init(SniperNetworkClient*c,const SniperNetworkClientHost*h){memset(c,0,sizeof(*c));if(h)c->host=*h;sniper_network_transport_init(&c->transport,&c->host.transport);}
void sniper_network_client_clear(SniperNetworkClient*c){if(c){sniper_network_transport_clear(&c->transport);memset(c,0,sizeof(*c));}}
int sniper_network_client_send(SniperNetworkClient*c,const SniperGamePacket*p){if(!c||!p||p->length>sizeof(p->data))return 0;return sniper_network_send_packet(&c->transport.sender,p->data,(int32_t)p->length,p->reliable,p->copy);}
int sniper_network_client_login(SniperNetworkClient*c,const char*name,const char*pass,int create){SniperGamePacket p;if(!c||!sniper_login_packet(&p,name,pass,create)||!sniper_network_client_send(c,&p))return 0;c->login_pending=1;c->login_valid=0;c->login_started=now(c)/10;return 1;}
int sniper_network_client_join(SniperNetworkClient*c,int32_t ship,int32_t param,int32_t audio,int32_t w,int32_t h){SniperGamePacket p;sniper_login_join_packet(&p,ship,param,audio,w,h);if(!sniper_network_client_send(c,&p))return 0;c->game.ready=0;c->game.player_id=-1;memset(c->cached,0,sizeof(c->cached));c->join_sent=1;return 1;}
int sniper_network_client_request_map(SniperNetworkClient*c){SniperGamePacket p;sniper_login_map_request(&p);if(!sniper_network_client_send(c,&p))return 0;memset(&c->download,0,sizeof(c->download));c->download_active=1;return 1;}
int sniper_network_client_request_arenas(SniperNetworkClient*c){SniperGamePacket p;sniper_login_arena_request(&p);if(!sniper_network_client_send(c,&p))return 0;memset(&c->download,0,sizeof(c->download));c->download_active=1;return 1;}
int sniper_network_client_cancel_download(SniperNetworkClient*c){SniperGamePacket p;sniper_login_cancel_download(&p);int result=sniper_network_client_send(c,&p);c->download_active=0;return result;}
int sniper_network_client_poll(SniperNetworkClient*c){
    if(!c)return -1;
    SniperGameReceiveHost h={c,game_clock,event,NULL};size_t n;const uint8_t*p;
    while((p=sniper_network_transport_receive(&c->transport,&n))){
        c->game.clock_offset=(uint32_t)c->transport.receiver.clock_offset;
        if(c->login_pending&&n&&p[0]==10){int r=sniper_login_reply(&c->login,p,n);if(r<0)return -1;c->login_pending=0;c->login_valid=1;if(c->host.login_reply)c->host.login_reply(c->host.user,&c->login);continue;}
        if(c->download_active&&n&&(p[0]==12||p[0]==13)){
            if(sniper_login_download(&c->download,p,n,write_at,c)<0)return -1;
            if(c->download.done){c->download_active=0;if(c->host.download_done)c->host.download_done(c->host.user,&c->download);}continue;
        }
        if(!sniper_game_receive_packet(&c->game,&h,p,n))return -1;
    }
    c->game.clock_offset=(uint32_t)c->transport.receiver.clock_offset;
    if(c->transport.invalid_packet)return -1;
    if(!c->transport.sender.connected)return -2;
    if(c->login_pending&&!sniper_network_transport_alive(&c->transport,1))return -2;
    return c->game.ready!=0;
}
void sniper_network_client_position(SniperNetworkClient*c,const int32_t a[13]){SniperGamePacket p;sniper_packet_position(&p,a,now(c),(uint32_t)c->transport.receiver.clock_offset);(void)sniper_network_client_send(c,&p);}
void sniper_network_client_death(SniperNetworkClient*c,int32_t kind,int32_t player){SniperGamePacket p;sniper_packet_death(&p,kind,player);(void)sniper_network_client_send(c,&p);sniper_network_transport_reset_ping(&c->transport);}
int sniper_network_client_chat(SniperNetworkClient*c,int32_t to,const char*text,uint32_t reliable){SniperGamePacket p;if(!sniper_packet_chat(&p,to,text,reliable))return 0;int ok=sniper_network_client_send(c,&p);SniperGameEvent e={0};e.kind=SNIPER_NET_CHAT;e.count=3;e.args[1]=c->game.player_id;e.args[2]=to;e.data=(const uint8_t*)text;e.size=strlen(text)+1;event(c,&e);return ok;}
void sniper_network_client_pickup(SniperNetworkClient*c,int32_t x,int32_t y,int32_t prize,uint32_t reliable){SniperGamePacket p;sniper_packet_pickup(&p,x,y,prize,c->game.player_id,reliable);(void)sniper_network_client_send(c,&p);SniperGameEvent e={0};e.kind=SNIPER_NET_PICKUP;e.count=4;e.args[0]=x;e.args[1]=y;e.args[2]=c->game.player_id;e.args[3]=prize;event(c,&e);}
void sniper_network_client_ship(SniperNetworkClient*c,int32_t ship){SniperGamePacket p;sniper_packet_ship(&p,ship);(void)sniper_network_client_send(c,&p);}
void sniper_network_client_team(SniperNetworkClient*c,int32_t team){SniperGamePacket p;sniper_packet_team(&p,team);(void)sniper_network_client_send(c,&p);SniperGameEvent e={0};e.kind=SNIPER_NET_TEAM;e.count=2;e.args[0]=c->game.player_id;e.args[1]=team;event(c,&e);}
void sniper_network_client_attach(SniperNetworkClient*c,int32_t target){SniperGamePacket p;sniper_packet_attach(&p,target);(void)sniper_network_client_send(c,&p);SniperGameEvent e={0};e.kind=SNIPER_NET_ATTACH;e.count=2;e.args[0]=c->game.player_id;e.args[1]=target;event(c,&e);}
void sniper_network_client_release_children(SniperNetworkClient*c,int32_t team){SniperGamePacket p[2];sniper_packet_release_children(p,team);(void)sniper_network_client_send(c,p);(void)sniper_network_client_send(c,p+1);}
int sniper_network_client_cached(SniperNetworkClient*c,int32_t to,int32_t slot,const uint8_t*p,size_t n){
    if(!c||!p||slot<0||slot>=16||n>INT32_MAX-4)return 0;
    size_t cap=c->cached[slot]?4:n+4;uint8_t*out=malloc(cap);if(!out)return 0;uint32_t copy=0;size_t length=sniper_packet_cached_into(out,cap,&copy,to,slot,p,n,&c->cached[slot]);if(!length){free(out);return 0;}
    int ok=sniper_network_send_packet(&c->transport.sender,out,(int32_t)length,1,(int)copy);free(out);SniperGameEvent e={0};e.kind=SNIPER_NET_DATA;e.count=3;e.args[0]=c->game.player_id;e.args[2]=(int32_t)n;e.data=p;e.size=n;event(c,&e);return ok;
}
