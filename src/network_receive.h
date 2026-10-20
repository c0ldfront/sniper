#ifndef SNIPER_NETWORK_RECEIVE_H
#define SNIPER_NETWORK_RECEIVE_H
#include <stdint.h>
#include <stddef.h>
typedef struct { int32_t sequence; uint8_t data[512]; int32_t length; } SniperReceiveSlot;
typedef struct {
    int32_t replies,best_rtt,clock_offset,expected,closed;
    SniperReceiveSlot slots[16];
    uint8_t bundle[1024]; int32_t bundle_length,bundle_cursor;
    uint8_t *fragment,*completed; size_t fragment_length,completed_length;
} SniperNetworkReceive;
typedef struct {
    void *user;
    uint32_t (*clock_ms)(void *);
    void (*send)(void *,const uint8_t *,size_t);
    void (*ack)(void *,uint16_t,uint32_t);
    void (*close)(void *);
    /* Takes borrowed compressed data; returns malloc-compatible owned result. */
    uint8_t *(*decompress)(void *,const uint8_t *,size_t,size_t *);
} SniperReceiveHost;
/* 0 application packet, 1 consumed control, -1 invalid/allocation failure.
 * Original assumes well-formed packets; bounds checks are explicit adaptations. */
void sniper_network_receive_init(SniperNetworkReceive *);
int sniper_network_receive_control(SniperNetworkReceive *,const uint8_t *,size_t,const SniperReceiveHost *);
/* Priority: bundle, completed fragment, then in-order reliable queue.
 * Returned pointer remains borrowed until next state-changing call. */
/* Transport probes buffered(), then socket, then reliable() only if socket empty. */
const uint8_t *sniper_network_receive_buffered(SniperNetworkReceive *,size_t *);
const uint8_t *sniper_network_receive_reliable(SniperNetworkReceive *,size_t *);
const uint8_t *sniper_network_receive_pop(SniperNetworkReceive *,size_t *);
/* Call once at the start of each public receive operation, matching 40193e. */
void sniper_network_receive_begin(SniperNetworkReceive *);
void sniper_network_receive_clear(SniperNetworkReceive *);
#endif
