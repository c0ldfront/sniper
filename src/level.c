#include "level.h"
SniperTile sniper_tile_decode(const uint8_t b[4]) {
    uint32_t word = (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
                    ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    SniperTile tile = {(uint16_t)(word & 0xfff),
                       (uint16_t)((word >> 12) & 0xfff), (uint8_t)(word >> 24)};
    return tile;
}
size_t sniper_level_record_count(size_t bytes) { return bytes / 4; }
uint32_t sniper_level_checksum(const uint8_t *data, size_t bytes) {
    uint32_t sum = 0;
    size_t consumed = sniper_level_record_count(bytes) * 4;
    for (size_t i = 0; i < consumed; ++i) sum += data[i];
    return sum;
}
