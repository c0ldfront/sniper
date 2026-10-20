#ifndef SNIPER_CHAT_RENDER_H
#define SNIPER_CHAT_RENDER_H
#include <stdint.h>
/*408e70 history entries are newest first, 128-byte slots; colors parallel. */
typedef struct { int32_t width,height,max_chars; } SniperChatLayout;
typedef struct { int32_t left,top,right,bottom; } SniperChatRect;
typedef void (*SniperChatText)(void *user,const char *text,int32_t x,int32_t y,int32_t color);
int sniper_chat_layout(int32_t screen_width,int32_t rows,SniperChatLayout *out);
void sniper_chat_history_draw(const SniperChatLayout *,const char (*lines)[128],const int32_t *colors,int32_t rows,SniperChatText,void *user);
/* The original uses cached surface dimensions, which can differ from new settings. */
void sniper_chat_placement(int32_t screen_height,int32_t cached_width,int32_t cached_height,const char *input,SniperChatRect *destination,SniperChatText draw_input,void *user);
/* Signed elapsed comparison after unsigned milliseconds/10 and wrapped subtraction. */
int sniper_chat_ticker_due(uint32_t milliseconds,uint32_t last_build_tick,int32_t interval,int32_t enabled);
#endif
