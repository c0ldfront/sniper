#include "font.h"
#include <string.h>
const SniperSprite sniper_fonts[4]={
    {139,0,0,6,8,2,48,0,0},{139,0,16,6,8,2,48,0,0},
    {139,0,32,6,8,2,48,0,0},{139,0,48,6,8,2,48,0,0}
};
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
int32_t sniper_font_width(const char *text){return bits((uint32_t)strlen(text)*6u);}
int32_t sniper_font_draw(const char *text,int32_t x,int32_t y,int color,SniperGlyphDraw draw,void *ctx){
    int32_t width=sniper_font_width(text);
    for(const unsigned char *p=(const unsigned char *)text;*p;++p){
        int32_t byte=*p<128 ? *p : (int32_t)*p-256;
        draw(ctx,color,x,y,byte-32);x=bits((uint32_t)x+6u);
    }
    return width;
}
