#include "projectile_explosion.h"
#include <string.h>
static int32_t bits(uint32_t x){int32_t r;memcpy(&r,&x,4);return r;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return bits((uint32_t)a*(uint32_t)b);}
static int32_t distance(int32_t square){
    unsigned short saved,truncating;int64_t result;
    __asm__ volatile("fnstcw %0":"=m"(saved));truncating=saved|0x0c00;
    __asm__ volatile("fildl %1; fsqrt; fldcw %2; fistpq %0; fldcw %3"
        :"=m"(result):"m"(square),"m"(truncating),"m"(saved):"st");
    return bits((uint32_t)result);
}
#define P(o) p->word[(o)/4]
void sniper_projectile_explode(SniperProjectile *p,SniperExplosionWorld *w){
    P(0x18)=132;w->sound(w->user,P(4)/1000,P(8)/1000);
    int32_t dx=sub(P(4),w->local->word[0x48/4])/1000;
    int32_t dy=sub(P(8),w->local->word[0x4c/4])/1000;
    int32_t dist=distance(add(mul(dx,dx),mul(dy,dy)));
    int32_t radius=mul(add(P(0x38),1),w->radius_per_level);
    if(radius>dist){
        w->damage(w->user,w->local,mul(sub(radius,dist),w->maximum_damage)/radius,P(0));
        w->shake=w->shake_duration;
    }
    if(add(w->projectile_count,P(0x44))>=512)return;
    int32_t angle=0;
    for(int32_t i=0;i<P(0x44);++i){
        int32_t args[15]={P(0),P(0x28),P(0x2c),0,0,angle/P(0x44),
            w->shrapnel_speed,P(0x3c),P(0x40),0,0,0,w->next_group,P(0x20),25};
        SniperProjectile *child=w->create(w->user,args);
        angle=add(angle,40000);w->next_group=add(w->next_group,1);
        w->projectiles[w->projectile_count++]=child;
    }
}
