#ifndef SNIPER_DISPLAY_INPUT_H
#define SNIPER_DISPLAY_INPUT_H
#include <stdint.h>
/*40591a/405971/405d46. Call AFTER chat macro and voice input consume shifted
 * keys. Does not consume key/repeat itself. Label mode starts1 each game;
 * stars/ticker are process options, shared with menu and persisted settings. */
void sniper_display_input(int32_t key,int32_t *label_mode,int32_t *stars,int32_t *ticker);
#endif
