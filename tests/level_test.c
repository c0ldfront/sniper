#include "level.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    const uint8_t first[] = {0x58, 0x40, 0x06, 0xd8};
    SniperTile t = sniper_tile_decode(first);
    assert(t.x == 88 && t.y == 100 && t.tile == 216);
    const uint8_t maximum[] = {255,255,255,255,99};
    t = sniper_tile_decode(maximum);
    assert(t.x == 4095 && t.y == 4095 && t.tile == 255);
    assert(sniper_level_record_count(7) == 1);
    assert(sniper_level_checksum(maximum, 5) == 1020);
    assert(sniper_level_checksum(NULL, 0) == 0);
    for (int a = 1; a < argc; ++a) {
        FILE *f = fopen(argv[a], "rb"); assert(f);
        assert(fseek(f, 0, SEEK_END) == 0); long size = ftell(f); assert(size >= 0);
        rewind(f); uint8_t *b = malloc((size_t)size); assert(b);
        assert(fread(b, 1, (size_t)size, f) == (size_t)size); fclose(f);
        size_t n = sniper_level_record_count((size_t)size);
        unsigned seen[256] = {0};
        for (size_t i = 0; i < n; ++i) {
            t = sniper_tile_decode(b + 4*i); assert(t.x < 1024 && t.y < 1024);
            ++seen[t.tile];
        }
        printf("%s: %zu records, checksum %u\n", argv[a], n,
               sniper_level_checksum(b, (size_t)size)); free(b);
    }
    return 0;
}
