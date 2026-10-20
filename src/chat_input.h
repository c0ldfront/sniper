#ifndef SNIPER_CHAT_INPUT_H
#define SNIPER_CHAT_INPUT_H
#include <stdint.h>
typedef struct {
    char text[82],macros[8][82];
    int32_t enabled,dirty;
} SniperChatInput;
/* Original40526d..40535b,405508..40555f,405952. submit borrows text
 * synchronously. Returns1 only when a Shift+F1..F8 macro consumes key/repeat. */
int sniper_chat_input(SniperChatInput *,int32_t width,uint8_t character,
    int32_t key,int32_t shift,void (*submit)(void *,const char *),void *user);
#endif
