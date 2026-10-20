#ifndef SNIPER_NOTICES_H
#define SNIPER_NOTICES_H
#include "attachment.h"
typedef SniperAttachmentHud SniperNotices;
typedef void (*SniperNoticeDraw)(void *,const char *,int32_t x,int32_t y,int32_t color);
/*407b50: null-terminated text must fit127 bytes; original copy is unbounded. */
void sniper_notice_push(SniperNotices *,const char *,int32_t ticks,int32_t color);
void sniper_notices_step(SniperNotices *,int32_t ticks);
/*4025c9..402626; retains empty slot gaps,coordinates from original globals. */
void sniper_notices_draw(const SniperNotices *,int32_t horizontal_extent,
    int32_t vertical_extent,SniperNoticeDraw,void *);
#endif
