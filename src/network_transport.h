#ifndef SNIPER_NETWORK_TRANSPORT_H
#define SNIPER_NETWORK_TRANSPORT_H
#include "network_send.h"
#include "network_receive.h"
typedef struct {
    void *user;
    uint32_t (*clock_ms)(void *);
    int32_t (*read)(void *,uint8_t *,int32_t,int peek);
    int32_t (*send)(void *,const uint8_t *,int32_t);
    void (*close)(void *);
    uint8_t *(*compress)(void *,const uint8_t *,int32_t,int32_t *);
    uint8_t *(*decompress)(void *,const uint8_t *,size_t,size_t *);
} SniperTransportHost;
typedef struct {
    SniperNetworkSend sender;
    SniperNetworkReceive receiver;
    SniperTransportHost host;
    uint8_t socket_buffer[512];
    uint32_t last_receive_tick,bytes_received,packets_received;
    int invalid_packet;
} SniperNetworkTransport;
/* No socket creation: host supplies an already connected datagram transport. */
void sniper_network_transport_init(SniperNetworkTransport *,const SniperTransportHost *);
void sniper_network_transport_clear(SniperNetworkTransport *);
/* Original401920: borrowed application payload or NULL if unavailable/closed.
 * Consume before next receive. Malformed input sets invalid_packet. */
const uint8_t *sniper_network_transport_receive(SniperNetworkTransport *,size_t *);
#endif
