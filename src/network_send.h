#ifndef SNIPER_NETWORK_SEND_H
#define SNIPER_NETWORK_SEND_H
#include <stdint.h>
#include <stddef.h>
typedef struct { uint8_t *data; int32_t size; uint32_t sequence,last_sent; } SniperNetworkPending;
typedef struct { uint8_t *data; int32_t size,offset,compressed; } SniperNetworkLarge;
typedef struct {
    int32_t connected,closing;
    uint32_t ping_tick;
    int32_t ping_min,ping_sent,ping_received,bundle_threshold,bundle_size;
    uint8_t bundle[1024];
    uint32_t next_sequence;
    int32_t head,count,large_count,pumping;
    SniperNetworkPending pending[256];
    SniperNetworkLarge large[16];
    uint32_t bytes_sent,packets_sent;
    uint32_t (*clock)(void *); /* original GetTickCount()/10, sampled per call */
    int32_t (*send)(void *,const uint8_t *,int32_t);
    /* Allocated compressed bytes must be free()-compatible. Optional; a
       compressed large submission fails when unavailable. */
    uint8_t *(*compress)(void *,const uint8_t *,int32_t,int32_t *);
    void *user;
} SniperNetworkSend;
void sniper_network_send_init(SniperNetworkSend *);
void sniper_network_send_close(SniperNetworkSend *);
int sniper_network_send_packet(SniperNetworkSend *,const uint8_t *,int32_t,int reliable,int compress);
void sniper_network_send_probe(SniperNetworkSend *);
void sniper_network_send_pump(SniperNetworkSend *);
void sniper_network_send_ack(SniperNetworkSend *,uint16_t index,uint32_t sequence);
#endif
