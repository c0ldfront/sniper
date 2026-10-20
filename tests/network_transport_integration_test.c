/* Controlled two-peer transport: actual codec, queue pressure, loss/retries. */
#include "../src/network_transport.h"
#include "../src/network_compression.h"
#include "../src/network_lifecycle.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct {uint8_t data[1024];int32_t size;} Datagram;
typedef struct Peer {struct Peer *other;Datagram q[1024];unsigned head,count;uint32_t ms;unsigned sent,reliable,dropped;int loss;SniperNetworkTransport transport;} Peer;
static uint32_t clock_ms(void*u){return ((Peer*)u)->ms;}
static int32_t send_packet(void*u,const uint8_t*p,int32_t n){Peer*s=u;assert(n>=0&&n<=1024);s->sent++;if(n>=8&&!p[0]&&p[1]==2){s->reliable++;if(s->loss&&s->reliable%7==1){s->dropped++;return n;}}Peer*t=s->other;assert(t->count<1024);Datagram*d=&t->q[(t->head+t->count++)%1024];memcpy(d->data,p,(size_t)n);d->size=n;return n;}
static int32_t read_packet(void*u,uint8_t*p,int32_t n,int peek){Peer*s=u;if(!s->count)return -1;Datagram*d=&s->q[s->head];assert(n>=d->size);memcpy(p,d->data,(size_t)d->size);int size=d->size;if(!peek){s->head=(s->head+1)%1024;s->count--;}return size;}
static uint8_t *decode(void*u,const uint8_t*p,size_t n,size_t*out){(void)u;int32_t z;assert(n<0x7fffffff);uint8_t*r=sniper_network_decompress(p,(int32_t)n,&z);*out=r?(size_t)z:0;return r;}
static void init(Peer*s,Peer*t){memset(s,0,sizeof(*s));s->other=t;SniperTransportHost h={s,clock_ms,read_packet,send_packet,NULL,sniper_network_compress_callback,decode};sniper_network_transport_init(&s->transport,&h);s->transport.sender.ping_received=10;}
static unsigned shutdown_deliveries;
static void wait_peer(void*u){Peer*a=u;Peer*b=a->other;a->ms+=100;b->ms=a->ms;size_t n;const uint8_t*p;while((p=sniper_network_transport_receive(&b->transport,&n))){assert(n==3&&!memcmp(p,"abc",3));shutdown_deliveries++;}}
int main(void){Peer*a=calloc(1,sizeof(*a)),*b=calloc(1,sizeof(*b));assert(a&&b);
    for(int compressed=0;compressed<2;compressed++){
        init(a,b);init(b,a);a->loss=b->loss=1;uint8_t expected[3][8192];size_t sizes[3]={8192,1440,501};
        for(int k=0;k<3;k++){for(size_t i=0;i<sizes[k];i++)expected[k][i]=(uint8_t)(1+(i*7+i/23+k)%9);assert(sniper_network_send_packet(&a->transport.sender,expected[k],(int32_t)sizes[k],1,compressed));}
        unsigned received=0;for(int step=0;step<5000;step++){
            a->ms=b->ms=(uint32_t)step*100;size_t n;const uint8_t*p;
            while((p=sniper_network_transport_receive(&b->transport,&n))){assert(received<3);assert(n==sizes[received]);assert(!memcmp(p,expected[received],n));received++;}
            while((p=sniper_network_transport_receive(&a->transport,&n))){(void)p;assert(0);}
            assert(!a->transport.invalid_packet&&!b->transport.invalid_packet);
            if(received==3&&!a->transport.sender.count&&!a->transport.sender.large_count)break;
        }
        assert(received==3&&a->dropped>0&&!a->transport.sender.count&&!a->transport.sender.large_count);
        sniper_network_transport_clear(&a->transport);sniper_network_transport_clear(&b->transport);
    }
    init(a,b);init(b,a);a->loss=1;
    assert(sniper_network_send_packet(&a->transport.sender,(const uint8_t*)"abc",3,1,0));
    sniper_network_transport_shutdown(&a->transport,wait_peer,a);
    assert(shutdown_deliveries==1&&!a->transport.sender.connected&&!a->transport.sender.count);
    sniper_network_transport_clear(&b->transport);
    free(a);free(b);puts("PASS two-peer raw/compressed ordered large transfers with actual codec, deterministic loss/retries, ACK drain, graceful shutdown recovery and sanitizer cleanup");return 0;
}
