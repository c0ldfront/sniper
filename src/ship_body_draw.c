#include "ship_body_draw.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
void sniper_ship_body_draw(SniperShipObject *o,int32_t local,int field94,
    int32_t cx,int32_t cy,SniperBodyDraw draw,void *ctx){
    int32_t *s=o->word;
    int32_t x=sub(s[0x48/4]/1000,cx),y=sub(s[0x4c/4]/1000,cy);
    if(s[0x60/4]>0){draw(ctx,SNIPER_BODY_DEATH,x,y,sub(102,s[0x60/4])/3);return;}
    if(!field94){x=bits((uint32_t)x+(uint32_t)s[0xb4/4]);y=bits((uint32_t)y+(uint32_t)s[0xb8/4]);}
    int32_t frame=s[0x58/4]/1000;
    if(s[0x6c/4]==-1){
        if(s[0xbc/4]<=24){
            s[0xc0/4]=0;
            draw(ctx,s[0x84/4] ? SNIPER_BODY_OVERRIDE : s[0x40/4],x,y,frame);
        }
        if(s[0xbc/4]>0)draw(ctx,SNIPER_BODY_SPAWN,x,y,sub(48,s[0xbc/4])/3);
    }else draw(ctx,s[0x44/4]==local ? SNIPER_BODY_ATTACHED_LOCAL : SNIPER_BODY_ATTACHED_REMOTE,x,y,frame);
}
