#ifndef SNIPER_SURFACE_FILL_H
#define SNIPER_SURFACE_FILL_H
#include <stdint.h>
typedef struct {int32_t left,top,right,bottom;} SniperFillRect;
typedef struct {uint8_t *pixels;int32_t pitch,width,height;} SniperFillBuffer;
typedef uint32_t (*SniperFillBlt)(void *,const SniperFillRect *,uint32_t color);
typedef uint32_t (*SniperFillLock)(void *,SniperFillBuffer *);
typedef uint32_t (*SniperFillUnlock)(void *,uint8_t *);
typedef uint32_t (*SniperFillRestore)(void *);
typedef struct {
    SniperFillBlt blt;SniperFillLock lock;SniperFillUnlock unlock;
    SniperFillRestore restore;void *user;
} SniperFillSurface;
/* 415e50:320px backbuffer uses color-fill Blt;otherwise indexed byte fill.
 * Invalid memory rectangles return E_INVALIDARG after unlocking (original
 * did not validate bounds). No clipping is introduced. */
uint32_t sniper_surface_fill(const SniperFillSurface *,int backbuffer,int32_t screen_width,const SniperFillRect *,uint32_t);
uint32_t sniper_surface_vertical(const SniperFillSurface *,int,int32_t,int32_t x,int32_t y,int32_t length,uint32_t);
uint32_t sniper_surface_horizontal(const SniperFillSurface *,int,int32_t,int32_t x,int32_t y,int32_t length,uint32_t);
#endif
