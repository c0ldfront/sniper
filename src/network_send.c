#include "network_send.h"
#include <stdlib.h>
#include <string.h>
static void le32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static uint32_t now(SniperNetworkSend *s){return s->clock?s->clock(s->user):0;}
static int32_t emit(SniperNetworkSend*s,const uint8_t*p,int32_t n){return s->send?s->send(s->user,p,n):-1;}
void sniper_network_send_init(SniperNetworkSend*s){memset(s,0,sizeof(*s));s->next_sequence=1000;s->ping_min=9999999;}
void sniper_network_send_close(SniperNetworkSend*s){for(int i=0;i<256;i++){free(s->pending[i].data);s->pending[i].data=NULL;}for(int i=0;i<s->large_count;i++)free(s->large[i].data);s->count=s->large_count=s->bundle_size=s->connected=0;}
void sniper_network_send_probe(SniperNetworkSend*s){
    if(s->ping_received<10&&(s->ping_sent<16||now(s)-s->ping_tick>50)){
        uint8_t ping[6]={0,0};le32(ping+2,now(s));emit(s,ping,6);s->ping_tick=now(s);s->ping_sent++;s->bytes_sent+=6;s->packets_sent++;
    }
}
int sniper_network_send_packet(SniperNetworkSend*s,const uint8_t*p,int32_t n,int reliable,int compress){
    if(!s||!s->connected||n<0||(!p&&n))return 0;
    if(!s->closing&&(int32_t)(now(s)-s->ping_tick)>30000){s->ping_min=9999999;s->ping_received=s->ping_sent=0;}
    sniper_network_send_probe(s);
    if(n>500){
        if(s->large_count>=16)return 0;
        int32_t size=n;uint8_t *copy;
        if(compress){if(!s->compress)return 0;copy=s->compress(s->user,p,n,&size);}
        else{copy=malloc((size_t)n);if(copy)memcpy(copy,p,(size_t)n);}
        if(!copy||size<0){free(copy);return 0;}
        s->large[s->large_count++]=(SniperNetworkLarge){copy,size,0,compress};return 1;
    }
    s->bytes_sent+=(uint32_t)n;s->packets_sent++;
    if(reliable){
        if(s->count>=256){sniper_network_send_pump(s);if(s->count>=256)return 0;}
        /* Valid native ring state stays nonnegative. Reject corrupt indexes. */
        int i=(s->head+s->count)%256;if(i<0)return 0;
        uint8_t *copy=malloc((size_t)n+8);if(!copy)return 0;
        copy[0]=0;copy[1]=2;le32(copy+2,s->next_sequence);copy[6]=(uint8_t)i;copy[7]=(uint8_t)(i>>8);if(n)memcpy(copy+8,p,(size_t)n);
        s->pending[i]=(SniperNetworkPending){copy,n+8,s->next_sequence,now(s)-100000};s->count++;s->next_sequence++;
    }else if(s->count<32){
        if(s->bundle_threshold>n){
            if(!s->bundle_size){s->bundle[0]=0;s->bundle[1]=7;s->bundle_size=2;}
            if(s->bundle_size<0||s->bundle_size>1023-n)return 0;
            s->bundle[s->bundle_size++]=(uint8_t)n;if(n)memcpy(s->bundle+s->bundle_size,p,(size_t)n);s->bundle_size+=n;
            if(s->bundle_threshold<s->bundle_size){emit(s,s->bundle,s->bundle_size);s->bundle_size=0;}
        }else if(emit(s,p,n)!=n)return 0;
    }
    sniper_network_send_pump(s);return 1;
}
void sniper_network_send_pump(SniperNetworkSend*s){
    if(!s)return;
    if(!s->pumping){
        s->pumping=1;
        if(s->count<=1&&s->large_count){
            SniperNetworkLarge *q=s->large;int32_t n=q->size-q->offset;if(n>=480)n=480;
            if(n>=0){uint8_t chunk[482];chunk[0]=0;chunk[1]=q->compressed?6:5;if(n)memcpy(chunk+2,q->data+q->offset,(size_t)n);q->offset+=n;sniper_network_send_packet(s,chunk,n+2,1,1);
                if(!n){free(q->data);s->large_count--;memmove(q,q+1,(size_t)s->large_count*sizeof(*q));}}
        }
        s->pumping=0;
    }
    if(s->count){uint32_t tick=now(s);int limit=s->count<14?s->count:14;
        for(int j=0;j<limit;j++){int i=(s->head+j)%256;if(i<0)return;SniperNetworkPending*q=s->pending+i;uint32_t d=tick-q->last_sent;uint32_t magnitude=(int32_t)d<0?0u-d:d;
            if(q->data&&(int32_t)magnitude>150+500*j){emit(s,q->data,q->size);q->last_sent=tick;}}
    }
}
/* ACK state transitions filled from 401c25..401caf; native signed indexes are
   guarded here to avoid malformed packet memory access. */
void sniper_network_send_ack(SniperNetworkSend*s,uint16_t i,uint32_t sequence){
    if(!s||i>=256||s->pending[i].sequence!=sequence)return;
    free(s->pending[i].data);s->pending[i].data=NULL;
    while(s->count>0&&s->head>=0&&s->head<256&&!s->pending[s->head].data){s->head=(s->head+1)%256;s->count--;}
}
