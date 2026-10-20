#ifndef SNIPER_FRAME_CLOCK_H
#define SNIPER_FRAME_CLOCK_H
#include <stdint.h>
typedef struct {
    uint32_t previous; /* 42e198: raw GetTickCount()/10 */
    uint32_t elapsed;  /* 4218c8: clamped simulation delta */
    uint32_t tick;     /* 4218cc: accumulated simulation time */
} SniperFrameClock;
/* Original rebase sites update only previous, preserving simulation time. */
void sniper_frame_clock_rebase(SniperFrameClock *clock, uint32_t milliseconds);
void sniper_frame_clock_step(SniperFrameClock *clock, uint32_t milliseconds);
/* Native402844 resets only accumulator; previous/elapsed have process lifetime. */
void sniper_frame_clock_begin_session(SniperFrameClock *clock,const SniperFrameClock *process);
#endif
