#include "network_transport.h"
#include <string.h>
static uint32_t clock_ms(void *u){SniperNetworkTransport*t=u;return t->host.clock_ms?t->host.clock_ms(t->host.user):0;}
static uint32_t clock_tick(void *u){return clock_ms(u)/10;}
static int32_t send_packet(void *u,const uint8_t*p,int32_t n){SniperNetworkTransport*t=u;return t->host.send?t->host.send(t->host.user,p,n):-1;}
static void send_control(void*u,const uint8_t*p,size_t n){(void)send_packet(u,p,(int32_t)n);}
static void ack(void*u,uint16_t i,uint32_t seq){SniperNetworkTransport*t=u;sniper_network_send_ack(&t->sender,i,seq);}
static void close_socket(void*u){SniperNetworkTransport*t=u;t->sender.connected=0;if(t->host.close)t->host.close(t->host.user);}
static uint8_t *compress(void*u,const uint8_t*p,int32_t n,int32_t*out){SniperNetworkTransport*t=u;return t->host.compress?t->host.compress(t->host.user,p,n,out):NULL;}
static uint8_t *decompress(void*u,const uint8_t*p,size_t n,size_t*out){SniperNetworkTransport*t=u;return t->host.decompress?t->host.decompress(t->host.user,p,n,out):NULL;}
void sniper_network_transport_init(SniperNetworkTransport*t,const SniperTransportHost*h){
    memset(t,0,sizeof(*t));if(h)t->host=*h;sniper_network_send_init(&t->sender);sniper_network_receive_init(&t->receiver);
    t->sender.user=t;t->sender.clock=clock_tick;t->sender.send=send_packet;t->sender.compress=compress;t->sender.connected=1;t->last_receive_tick=clock_tick(t);
}
void sniper_network_transport_clear(SniperNetworkTransport*t){sniper_network_send_close(&t->sender);sniper_network_receive_clear(&t->receiver);}
const uint8_t *sniper_network_transport_receive(SniperNetworkTransport*t,size_t*n){
    if(!t||!n||!t->sender.connected)return NULL;
    sniper_network_receive_begin(&t->receiver);
    sniper_network_send_pump(&t->sender);
    SniperReceiveHost h={t,clock_ms,send_control,ack,close_socket,decompress};
    for(;;){
        if(!t->sender.connected)return NULL;
        t->receiver.replies=t->sender.ping_received;t->receiver.best_rtt=t->sender.ping_min;
        sniper_network_send_probe(&t->sender);
        const uint8_t *p=sniper_network_receive_buffered(&t->receiver,n);
        if(!p&&t->receiver.bundle_length>t->receiver.bundle_cursor){t->invalid_packet=1;return NULL;}
        if(!p){
            int32_t len=t->host.read?t->host.read(t->host.user,t->socket_buffer,512,1):-1;
            if(len>0){
                if(len>512){t->invalid_packet=1;return NULL;}
                t->last_receive_tick=clock_tick(t);
                (void)t->host.read(t->host.user,t->socket_buffer,len,0);
                p=t->socket_buffer;*n=(size_t)len;
            }else p=sniper_network_receive_reliable(&t->receiver,n);
        }
        if(!p)return NULL;
        t->bytes_received+=(uint32_t)*n;t->packets_received++;
        int handled=sniper_network_receive_control(&t->receiver,p,*n,&h);
        t->sender.ping_received=t->receiver.replies;t->sender.ping_min=t->receiver.best_rtt;
        if(handled<0){t->invalid_packet=1;return NULL;}
        if(!handled)return p;
    }
}
