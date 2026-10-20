#ifndef SNIPER_SCENE_FRAME_H
#define SNIPER_SCENE_FRAME_H
#include "ship_create.h"
typedef enum {
    SNIPER_SCENE_STARS,SNIPER_SCENE_BATCH_BEGIN,SNIPER_SCENE_MAP,
    SNIPER_SCENE_SPECIAL,SNIPER_SCENE_ACTORS,SNIPER_SCENE_HUD,
    SNIPER_SCENE_BATCH_FLUSH,SNIPER_SCENE_RADAR,SNIPER_SCENE_CHAT,
    SNIPER_SCENE_NOTICES,SNIPER_SCENE_MODE_ONE,SNIPER_SCENE_MODE_TWO,
    SNIPER_SCENE_MODE_THREE,SNIPER_SCENE_MODE_FOUR
} SniperSceneStage;
typedef struct {
    int32_t stars,special,radar,chat,mode;
    SniperShipObject *local;
    void (*stage)(void *,SniperSceneStage);
    uint32_t (*flip)(void *);
    int (*restore)(void *);
    void *user;
} SniperSceneFrame;
/*4024c3..402682: after cursor selection,for active render frames.
 * Callbacks required for all enabled stages. Return0 if lost-surface recovery
 * fails,1 on successful presentation. Other flip errors retry,as original.
 * Stage implementations own live actors,map,HUD and surface state. */
int sniper_scene_frame(SniperSceneFrame *);
#endif
