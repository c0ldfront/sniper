#ifndef SNIPER_NETWORK_CHECKSUM_H
#define SNIPER_NETWORK_CHECKSUM_H
#include <stdint.h>
#include <stddef.h>
/* Native401fa0: sum complete little-endian words modulo2^32, ignore tail. */
uint32_t sniper_network_checksum(const uint8_t *,size_t);
/* Native open failure returns0, indistinguishable from checksum0. */
uint32_t sniper_network_checksum_file(const char *);
#endif
