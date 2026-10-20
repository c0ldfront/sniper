#ifndef SNIPER_NETWORK_COMPRESSION_H
#define SNIPER_NETWORK_COMPRESSION_H
#include <stdint.h>
/* Returned storage is free-compatible; input is borrowed. NULL means invalid
   input/allocation failure. Padding bits are zeroed rather than heap residue. */
uint8_t *sniper_network_compress(const uint8_t *,int32_t,int32_t *,int bypass,uint32_t passes);
uint8_t *sniper_network_decompress(const uint8_t *,int32_t,int32_t *);
uint8_t *sniper_network_compress_callback(void *,const uint8_t *,int32_t,int32_t *);
#endif
