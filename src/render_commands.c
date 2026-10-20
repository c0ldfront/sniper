#include "render_commands.h"
#include "ship_assets.h"
#include "font.h"
#include <limits.h>
int sniper_resource_command(const SniperSprite *s,int32_t x,int32_t y,int32_t frame,
    int centered,int opaque,SniperRenderCommand *out){
    SniperRenderCommand c;
    if(!out || sniper_sprite_blit(s,x,y,frame,centered,opaque,&c.blit))return -1;
    int64_t left=(int64_t)c.blit.source.left+s->origin_x;
    int64_t top=(int64_t)c.blit.source.top+s->origin_y;
    int64_t right=(int64_t)c.blit.source.right+s->origin_x;
    int64_t bottom=(int64_t)c.blit.source.bottom+s->origin_y;
    if(left<INT32_MIN || top<INT32_MIN || right>INT32_MAX || bottom>INT32_MAX)return -1;
    c.resource=s->resource;c.blit.source=(SniperRect){(int32_t)left,(int32_t)top,(int32_t)right,(int32_t)bottom};
    *out=c;return 0;
}
static void emit(SniperCommandSink *sink,const SniperSprite *s,int32_t x,int32_t y,int32_t frame,int center){
    SniperRenderCommand command;
    if(sniper_resource_command(s,x,y,frame,center,0,&command)){sink->failed=1;return;}
    sink->submit(sink->user,&command);
}
void sniper_command_body(void *ctx,int slot,int32_t x,int32_t y,int32_t frame){
    SniperCommandSink *s=ctx;
    if(slot<0 || slot>=9){s->failed=1;return;}
    emit(s,&sniper_ship_assets[slot],x,y,frame,1);
}
void sniper_command_effect(void *ctx,int resource,int32_t x,int32_t y,int32_t frame){
    SniperCommandSink *s=ctx;
    if(resource!=147 && resource!=173){s->failed=1;return;}
    emit(s,&sniper_effect_assets[resource==173],x,y,frame,1);
}
static void glyph(void *ctx,int color,int32_t x,int32_t y,int32_t frame){
    SniperCommandSink *s=ctx;
    if(color<0 || color>=4){s->failed=1;return;}
    emit(s,&sniper_fonts[color],x,y,frame,0);
}
void sniper_command_text(void *ctx,const char *text,int32_t x,int32_t y,int color){
    sniper_font_draw(text,x,y,color,glyph,ctx);
}
