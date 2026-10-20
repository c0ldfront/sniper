#ifndef SNIPER_SPECTATOR_CONTROL_H
#define SNIPER_SPECTATOR_CONTROL_H
#include "local_control.h"
typedef struct {
    SniperLocalControl *input;
    SniperShipObject **ordered;
    int32_t count,selected,tick,last_tick,watched;
    void (*position)(void *,const int32_t args[13]);
    void (*watch)(void *,int32_t id);
    void *user;
} SniperSpectatorControl;
/*405ed9..405f70 plus406a84 queued input cleanup. Nonnegative selected
 * bounds guard is a deliberate safety adaptation. Callbacks synchronous. */
void sniper_spectator_control(SniperSpectatorControl *);
#endif
