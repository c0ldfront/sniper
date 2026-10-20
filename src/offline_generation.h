#ifndef SNIPER_OFFLINE_GENERATION_H
#define SNIPER_OFFLINE_GENERATION_H
#include <stdint.h>
/* 411450. Calls clock twice only when unsigned elapsed ticks >500.
 * Requires an empty cell in central264x264 region; original retries forever
 * when fully occupied. RNG state is shared with ship spawn/prize effects. */
void sniper_offline_generate(uint32_t *previous,uint32_t *random_state,
    const uint8_t *map,uint32_t (*clock_ms)(void *),
    void (*create)(void *,int32_t x,int32_t y,int32_t code,int32_t lifetime),void *user);
#endif
