#include "projectile_create.h"
#include <string.h>
static int32_t bits(uint32_t x){int32_t r;memcpy(&r,&x,4);return r;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
static int32_t absolute(int32_t a){return a<0?bits(0u-(uint32_t)a):a;}
static int32_t trunc_x87(long double value){
    unsigned short saved,truncating;int64_t result;
    __asm__ volatile("fnstcw %0":"=m"(saved));truncating=saved|0x0c00;
    __asm__ volatile("fldcw %1; fldt %2; fistpq %0; fldcw %3"
        :"=m"(result):"m"(truncating),"m"(value),"m"(saved):"st");
    return bits((uint32_t)result);
}
static void vector(long double angle,int32_t speed,int32_t yspeed,int32_t *x,int32_t *y){
    long double sx,cy;
    __asm__ volatile("fldt %2; fsin; fimull %3; fstpt %0; fldt %2; fcos; fimull %4; fstpt %1"
        :"=m"(sx),"=m"(cy):"m"(angle),"m"(speed),"m"(yspeed):"st");
    *x=trunc_x87(sx);*y=trunc_x87(cy);
}
static void repel(int32_t dx,int32_t dy,int32_t speed,int32_t *x,int32_t *y){
    long double angle;
    __asm__ volatile("fildl %1; fildl %2; fpatan; fstpt %0":"=m"(angle):"m"(dx),"m"(dy):"st");
    vector(angle,speed,speed,x,y);
}
#define P(o) p->word[(o)/4]
void sniper_projectile_create(SniperProjectile *p,SniperProjectileCreateWorld *w,const int32_t a[15]){
    P(0)=a[0];P(4)=a[1];P(8)=a[2];P(0x34)=a[7];P(0x38)=a[8];
    P(0x3c)=a[9];P(0x40)=a[10];P(0x44)=a[11];P(0x1c)=a[12];P(0x20)=a[13];
    P(0x14)=0;P(0x18)=0;P(0x24)=0;P(0x30)=a[14];
    long double angle=(long double)a[5]*(double)0x1.496b7c539d3fbp-13;
    vector(angle,a[6],bits(0u-(uint32_t)a[6]),&P(0xc),&P(0x10));
    P(0xc)=add(P(0xc),a[3]);P(0x10)=add(P(0x10),a[4]);
    if(a[14])return;
    if(a[7]!=5){w->sound(w->user,(a[7]==3 || a[7]==4)?w->bomb_sounds[a[8]]:w->bullet_sounds[a[8]],a[1]/1000,a[2]/1000);return;}
    w->sound(w->user,w->repel_sound,a[1]/1000,a[2]/1000);P(0x18)=50;
    SniperShipObject *s=w->local;
    if(s->word[0x68/4]!=P(0x20)){
        int32_t x=s->word[0x48/4]/16000,y=s->word[0x4c/4]/16000;
        int safe=x>=0 && y>=0 && x<1024 && y<1024 && w->map[y*1024+x]==152;
        int32_t dx=sub(s->word[0x48/4],P(4)),dy=sub(s->word[0x4c/4],P(8));
        if(!safe && absolute(dx)/1000<w->repel_range && absolute(dy)/1000<w->repel_range){
            repel(dx,dy,w->repel_speed,&s->word[0x50/4],&s->word[0x54/4]);w->repel_timer=w->repel_duration;
        }
    }
    for(int32_t i=0;i<w->projectile_count;++i){
        SniperProjectile *q=w->projectiles[i];
        if(q->word[0x20/4]==P(0x20))continue;
        int32_t dx=sub(q->word[1],P(4)),dy=sub(q->word[2],P(8));
        if(absolute(dx)/1000<w->repel_range && absolute(dy)/1000<w->repel_range)
            repel(dx,dy,w->repel_speed,&q->word[3],&q->word[4]);
    }
}
