#ifndef SNIPER_FRAME_UPDATE_H
#define SNIPER_FRAME_UPDATE_H
#include "frame_clock.h"
#include "camera.h"
typedef struct {
    SniperFrameClock clock;
    SniperShipObject *local,*watched;
    int32_t auto_pending,special,safe_timer,frequency,frequency_count;
    int32_t width,height,tile_x,tile_y,shake;
    uint32_t random_state;
    SniperCamera camera;
    void (*post_command)(void *); /* PostMessage(window,0x401,0,0) */
    void (*backend)(void *);
    void (*input)(void *);
    void (*team)(void *,int32_t frequency);
    void (*detach)(void *,int32_t target);
    void (*simulate)(void *);
    void *user;
} SniperFrameUpdate;
/*4020b5..402468,after clock read through camera update. Callbacks may mutate
 * state;later branches read it live. frequency_count must be positive when
 * ordinary simulation is enabled. Backend/input run even in special mode.
 * Rendering/cursor and message pump are outside this slice. */
void sniper_frame_update(SniperFrameUpdate *,uint32_t milliseconds);
#endif
