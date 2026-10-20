#ifndef SNIPER_DISPLAY_RECOVERY_H
#define SNIPER_DISPLAY_RECOVERY_H
#include <stdint.h>
typedef struct {
    uintptr_t primary,backbuffer,team_panel,equipment_panel,palette,tile_object,tile_surface;
    int32_t dirty[3]; /*428334,4289f4,428188*/
    uint32_t colors[16];
    uint32_t (*restore)(void *,uintptr_t);
    void (*release)(void *,uintptr_t);
    uint32_t (*set_palette)(void *,uintptr_t,uintptr_t);
    uint32_t (*restore_tiles)(void *,uintptr_t);
    uint32_t (*resolve_color)(void *,uintptr_t,uint32_t);
    void *user;
} SniperDisplayRecovery;
/*403e40: primary/backbuffer nonzero Restore results abort;other results are
 * ignored as original. Rebuilds caches lazily by releasing their surfaces. */
int sniper_display_recover(SniperDisplayRecovery *);
extern const uint32_t sniper_display_color_rgb[16];
#endif
