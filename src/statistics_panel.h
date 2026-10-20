#ifndef SNIPER_STATISTICS_PANEL_H
#define SNIPER_STATISTICS_PANEL_H
#include "text_panel.h"
#include "ship_create.h"
typedef struct {
    int32_t valid,scroll,count,per;
    uint32_t tick;
    char title[128],text[256][128];
    SniperPanelRow rows[256];
} SniperStatisticsPanel;
/*408ac0 content/cache/scroll. Positive count/history and valid nonnegative
 * selected index required. Returns1 rebuilt,0 retained,-1 invalid input.
 * Separate clock calls preserve cache-check versus post-build timestamps. */
int sniper_statistics_update(SniperStatisticsPanel *,SniperShipObject *const *,int32_t count,
    const SniperShipObject *local,int32_t history,int32_t *selected,int32_t *dirty,
    int compact,uint32_t (*clock_ms)(void *),void *user);
int32_t sniper_statistics_toggle(int32_t mode,int32_t *dirty);
#endif
