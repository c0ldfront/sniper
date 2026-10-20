#include "frame_clock.h"
void sniper_frame_clock_begin_session(SniperFrameClock *clock,const SniperFrameClock *process)
{
    *clock=*process;
    clock->tick=0;
}

void sniper_frame_clock_rebase(SniperFrameClock *clock, uint32_t milliseconds)
{
    clock->previous = milliseconds / 10u;
}

/* 4020b5..4020fb. Use unsigned arithmetic to preserve x86 wrap without
 * undefined signed overflow or implementation-defined signed conversion. */
void sniper_frame_clock_step(SniperFrameClock *clock, uint32_t milliseconds)
{
    uint32_t now = milliseconds / 10u;
    uint32_t delta = now - clock->previous;
    clock->previous = now;
    if (delta & UINT32_C(0x80000000)) delta = 0;
    if (delta > 50u) delta = 50u;
    clock->elapsed = delta;
    clock->tick += delta;
}
