#ifndef SNIPER_LIST_NAVIGATION_H
#define SNIPER_LIST_NAVIGATION_H
#include <stdint.h>
/*40562a..4056bd: PageUp/Down move selection; Shift adjusts history size.
 * Preserves x86 wrap and original signed bounds, including empty list=>-1
 * for PageDown. Returns0 on unrelated keys,1 on a recognized command. */
int sniper_list_navigation(int32_t key,int32_t shift,int32_t count,int32_t height,
    int32_t *selected,int32_t *history,int32_t *dirty);
#endif
