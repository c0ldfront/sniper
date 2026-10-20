#include "projectile_draw.h"
#include <string.h>
const SniperSprite sniper_projectile_assets[5]={
    {109,0,0,5,5,6,4,0,4},
    {145,0,0,16,16,3,10,0,10},
    {138,0,0,16,16,1,7,49,7},
    {146,0,0,80,80,11,4,132,44},
    {176,0,0,96,96,4,3,50,10}
};
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
#define P(o) p->word[(o)/4]
void sniper_projectile_draw(const SniperProjectile *p,int32_t cx,int32_t cy,
    int suppress,SniperProjectileDraw draw,void *user){
    if(P(4)==-1)return;
    int asset;int32_t frame,type=P(0x34),impact=P(0x18);
    if(impact>0){
        if(type==3 || type==4){asset=3;frame=sub(132,impact)/3;}
        else if(type==5){asset=4;frame=sub(50,impact)/5;}
        else {if(suppress)return;asset=2;frame=sub(49,impact)/7;}
    }else if(type==3 || type==4){
        asset=1;frame=bits((uint32_t)((P(0x14)/10)%10)+(uint32_t)P(0x38)*10u);
    }else{
        asset=0;frame=bits((uint32_t)((P(0x14)/6)%4)+(uint32_t)P(0x38)*4u+(type==1?0u:12u));
    }
    draw(user,asset,sub(P(4)/1000,cx),sub(P(8)/1000,cy),frame,1,0);
}
