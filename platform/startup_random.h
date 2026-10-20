#ifndef SNIPER87_STARTUP_RANDOM_H
#define SNIPER87_STARTUP_RANDOM_H
#include <stdint.h>
/* Original application's CRT4165e0/4165f0 stream. This explicit state must be
 * handed to gameplay; it is not the host C library's srand/rand state. */
typedef struct {uint32_t crt_state;int32_t frequency;} Sniper87StartupRandom;
void sniper87_startup_random_seed(Sniper87StartupRandom *,uint32_t epoch_seconds);
#ifdef _WIN32
/* Seeds and consumes one default-frequency draw even when INI value exists. */
void sniper87_startup_random_load(Sniper87StartupRandom *,uint32_t epoch_seconds);
void sniper87_startup_frequency_save(int32_t frequency);
#endif
#endif
