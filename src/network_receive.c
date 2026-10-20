#include "network_receive.h"
#include <stdlib.h>
#include <string.h>
static uint32_t rd(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void wr(uint8_t *p,uint32_t v){for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t now(const SniperReceiveHost *h){return h&&h->clock_ms?h->clock_ms(h->user)/10:0;}
static void send(const SniperReceiveHost *h,const uint8_t *p,size_t n){if(h&&h->send)h->send(h->user,p,n);}
void sniper_network_receive_init(SniperNetworkReceive *s){memset(s,0,sizeof(*s));s->expected=1000;s->best_rtt=9999999;}
int sniper_network_receive_control(SniperNetworkReceive *s,const uint8_t *p,size_t n,const SniperReceiveHost *h){
    if(!s||!p||!n)return -1;
    if(p[0])return 0;
    if(n<2)return -1;
    uint8_t out[10]={0}; int32_t seq;
    switch(p[1]){
    case 0:
        if(n<6)return -1;
        out[1]=1;memcpy(out+2,p+2,4);wr(out+6,now(h));send(h,out,10);break;
    case 1:{
        if(n<10)return -1;
        s->replies=(int32_t)((uint32_t)s->replies+1);
        uint32_t tick=now(h);int32_t rtt=(int32_t)(tick-rd(p+2));
        if(s->best_rtt>rtt){s->best_rtt=rtt;s->clock_offset=(int32_t)((uint32_t)(rtt/2)-tick+rd(p+6));}break;}
    case 2:{
        if(n<8||n-8>512)return -1;
        seq=(int32_t)rd(p+2);int free_slot=-1,duplicate=0;
        if(s->expected<=seq){
            for(int i=15;i>=0;i--){if(s->slots[i].sequence==seq){duplicate=1;break;}if(!s->slots[i].sequence)free_slot=i;}
            if(!duplicate){if(free_slot<0)break;SniperReceiveSlot *q=&s->slots[free_slot];q->sequence=seq;q->length=(int32_t)n-8;memcpy(q->data,p+8,n-8);}
        }
        out[1]=3;memcpy(out+2,p+2,6);send(h,out,8);break;}
    case 3:{
        if(n<8)return -1;
        int index=(int16_t)((unsigned)p[6]|(unsigned)p[7]<<8);
        if(index<0||index>=256)return -1;
        if(h&&h->ack)h->ack(h->user,(uint16_t)index,rd(p+2));
        break;}
    case 4:s->closed=1;if(h&&h->close)h->close(h->user);break;
    case 5:case 6:
        if(n==2){
            if(p[1]==6){if(!h||!h->decompress)return -1;size_t z=0;uint8_t *b=h->decompress(h->user,s->fragment,s->fragment_length,&z);if(!b)return -1;free(s->completed);s->completed=b;s->completed_length=z;/* Native decompressor consumes its input; adapter retains borrowed-input contract. */free(s->fragment);}
            else{free(s->completed);s->completed=s->fragment;s->completed_length=s->fragment_length;}
            s->fragment=NULL;s->fragment_length=0;
        }else{
            if(n-2>SIZE_MAX-s->fragment_length)return -1;
            uint8_t *b=realloc(s->fragment,s->fragment_length+n-2);if(!b)return -1;
            memcpy(b+s->fragment_length,p+2,n-2);s->fragment=b;s->fragment_length+=n-2;
        }break;
    case 7:if(n>sizeof(s->bundle))return -1;memcpy(s->bundle,p,n);s->bundle_length=(int32_t)n;s->bundle_cursor=2;break;
    default:break;
    }
    return 1;
}
const uint8_t *sniper_network_receive_buffered(SniperNetworkReceive *s,size_t *n){
    if(s->bundle_length>s->bundle_cursor){
        if(s->bundle_cursor<0)return NULL;
        int length=(int8_t)s->bundle[s->bundle_cursor];
        if(length<0||length>s->bundle_length-s->bundle_cursor-1)return NULL;
        const uint8_t *p=s->bundle+s->bundle_cursor+1;s->bundle_cursor+=length+1;*n=(size_t)length;return p;
    }
    if(s->completed){*n=s->completed_length;return s->completed;}
    return NULL;
}
const uint8_t *sniper_network_receive_reliable(SniperNetworkReceive *s,size_t *n){
    for(int i=0;i<16;i++)if(s->slots[i].sequence==s->expected){s->slots[i].sequence=0;s->expected=(int32_t)((uint32_t)s->expected+1);*n=(size_t)s->slots[i].length;return s->slots[i].data;}
    return NULL;
}
const uint8_t *sniper_network_receive_pop(SniperNetworkReceive *s,size_t *n){const uint8_t *p=sniper_network_receive_buffered(s,n);return p?p:sniper_network_receive_reliable(s,n);}
void sniper_network_receive_begin(SniperNetworkReceive *s){free(s->completed);s->completed=NULL;s->completed_length=0;}
void sniper_network_receive_clear(SniperNetworkReceive *s){if(s){free(s->fragment);free(s->completed);memset(s,0,sizeof(*s));}}
