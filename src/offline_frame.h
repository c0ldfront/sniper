#ifndef SNIPER_OFFLINE_FRAME_H
#define SNIPER_OFFLINE_FRAME_H
#include "frame_update.h"
#include "offline_control.h"
#include "offline_simulation.h"
#include "offline_team.h"
typedef struct SniperOfflineFrame {
    SniperFrameUpdate *frame;
    SniperOfflineCombat *combat;
    SniperOfflinePickups *pickups;
    SniperOrdinaryControl *control;
    int32_t selected,list_dirty;
    /* Optional complete input handler for commands/special/spectator modes.
     * Receives this binding;may mutate frame and combat state. NULL chooses
     * recovered ordinary control,which excludes those earlier input paths. */
    void (*input)(void *,struct SniperOfflineFrame *);
    void *input_user;
    void (*backend_override)(void *);
    void (*team_override)(void *,int32_t);
    void (*detach_override)(void *,int32_t);
    void (*simulate_override)(void *,const int32_t *);
    void *backend_user;
} SniperOfflineFrame;
/* All components must share combat->ships;pickups must be bound and non-NULL.
 * Post-command callback uses frame's original user;other frame handlers are
 * borrowed temporarily. frame's clock,frequency,camera and flags persist.
 * World.milliseconds is the supplied time snapshot;generation uses its own
 * configured clock callback. Returns0 before mutation for unsupported input
 * modes (special/type4 with no input override),otherwise1. No rendering/audio. */
int sniper_offline_frame(SniperOfflineFrame *,uint32_t milliseconds);
#endif
