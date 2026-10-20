#ifndef SNIPER_RENDER_COMMANDS_H
#define SNIPER_RENDER_COMMANDS_H
#include "sprite.h"
#include "ship_render.h"
typedef struct { int resource;SniperBlit blit; } SniperRenderCommand;
typedef void (*SniperSubmit)(void *,const SniperRenderCommand *);
typedef struct { SniperSubmit submit;void *user;int failed; } SniperCommandSink;
/* Source coordinates address the full bitmap resource rather than original
 * cropped DirectDraw surfaces. No scaling or palette conversion is implied. */
int sniper_resource_command(const SniperSprite *,int32_t x,int32_t y,int32_t frame,
    int centered,int opaque,SniperRenderCommand *);
void sniper_command_body(void *,int slot,int32_t x,int32_t y,int32_t frame);
void sniper_command_effect(void *,int resource,int32_t x,int32_t y,int32_t frame);
void sniper_command_text(void *,const char *,int32_t x,int32_t y,int color);
#endif
