#include "ship_labels.h"
#include <stdio.h>
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
void sniper_ship_labels(const SniperShipObject *o,const SniperShipObject *local,
    SniperShipObject *parent,int mode,int32_t upper,int32_t lower,int32_t bonus,
    int32_t x,int32_t y,SniperLabelDraw draw,void *ctx){
    const int32_t *s=o->word,*l=local->word;char text[128];
    if(s[0x44/4]==l[0x44/4]){
        int32_t energy=s[0x5c/4];
        if(s[0xc8/4]/3>energy && mode>0 && energy>=0 && s[0x6c/4]==-1){
            snprintf(text,sizeof text,"%d",energy/1000);
            draw(ctx,text,add(x,12),add(y,12),energy<=250000 ? 2 : 3);
        }
        return;
    }
    if(s[0x6c/4]==l[0x44/4] || s[0x60/4]>0)return;
    int32_t row=0;
    if(s[0x6c/4]!=-1 && parent){row=add(parent->word[0xc0/4],1);parent->word[0xc0/4]=row;}
    int32_t difference=bits((uint32_t)s[0xa0/4]-(uint32_t)l[0xa0/4]);
    if(difference>=upper)difference=upper;
    int32_t floor=bits(0u-(uint32_t)lower);
    if(difference<=floor)difference=floor;
    int32_t value=add(s[0xa0/4],add(difference,bonus));
    if(mode==1)snprintf(text,sizeof text,"%.31s(%d)",(const char *)s,value);
    else if(mode==2){
        snprintf(text,sizeof text,"%.31s,%.31s(%d)",(const char *)s,(const char *)s+32,value);
        text[32]=0;
    }else return;
    draw(ctx,text,add(x,12),add(add(y,bits((uint32_t)row*8u)),12),s[0x68/4]==l[0x68/4] ? 2 : 0);
}
