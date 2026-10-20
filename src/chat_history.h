#ifndef SNIPER_CHAT_HISTORY_H
#define SNIPER_CHAT_HISTORY_H
#include <stdint.h>
/*404c50: arrays contain rows slots of128 bytes/colors, newest at0. sender_name
 * is NULL for a missing or negative sender; otherwise the original ship name.
 * Caller resolves ID before this function. rows<3 suppresses receipt entirely.
 * Buffers must be nonaliasing and strings NUL terminated. */
void sniper_chat_history(char (*lines)[128],int32_t *colors,int32_t rows,
    int32_t width,int32_t *dirty,const char *text,const char *sender_name,
    int32_t recipient);
#endif
