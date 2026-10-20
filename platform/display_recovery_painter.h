#ifndef SNIPER_DISPLAY_RECOVERY_PAINTER_H
#define SNIPER_DISPLAY_RECOVERY_PAINTER_H
#include "scene_painter.h"
#include "../src/display_recovery.h"
/* Bind403e40 to a scene. state retains three invalidation flags and all16
 * palette values for future radar/chat bindings. Caller owns state and palette.
 * Returns original recovery bool;palette/atlas errors remain ignored. */
int sniper_scene_restore(SniperScenePainter *,LPDIRECTDRAWPALETTE,SniperDisplayRecovery *);
#endif
