#ifndef SNIPER87_LEVEL_H
#define SNIPER87_LEVEL_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint16_t x, y; uint8_t tile; } SniperTile;
/* Source evidence: SNIPER.EXE 0x4116d4..0x4116f0. */
SniperTile sniper_tile_decode(const uint8_t bytes[4]);
/* Source evidence: 0x40a2f9..0x40a356. Trailing partial records are ignored. */
size_t sniper_level_record_count(size_t bytes);
uint32_t sniper_level_checksum(const uint8_t *data, size_t bytes);
#endif
