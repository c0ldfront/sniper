#include "projectile_update.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
static int32_t neg(int32_t a){return bits(0u-(uint32_t)a);}
static int32_t abs32(int32_t a){return a<0?neg(a):a;}
static int solid(SniperProjectileWorld *w,int32_t x,int32_t y,int axis) {
    if((axis==0?x:y)<0)return 1;
    int32_t tx=x/16000,ty=y/16000;
    if(tx<0 || ty<0 || tx>=1024 || ty>=1024)return 1;
    int t=w->map[ty*1024+tx];return t!=0 && t!=152;
}
#define P(o) p->word[(o)/4]
int sniper_projectile_update(SniperProjectile *p,SniperProjectileWorld *w,int32_t ticks) {
    P(0x14)=add(P(0x14),ticks);P(0x30)=sub(P(0x30),ticks);
    for(int32_t step=0;step<ticks;++step) {
        if(P(0x18)>0){P(0x18)=sub(P(0x18),1);if(!P(0x18))return 1;continue;}
        int32_t oldx=P(4),oldy=P(8);P(0x28)=oldx;P(0x2c)=oldy;
        if(P(0xc)!=0 || P(0x10)!=0) {
            if(P(0x34)==2 && P(0x30)<=0) {
                P(4)=add(oldx,P(0xc));
                if(solid(w,P(4),oldy,0)){P(4)=oldx;P(0xc)=neg(P(0xc));}
                P(8)=add(oldy,P(0x10));
                if(solid(w,P(4),P(8),1)){P(8)=oldy;P(0x10)=neg(P(0x10));}
            }else{
                P(4)=add(oldx,P(0xc));int hit=solid(w,P(4),oldy,0);
                if(!hit){P(8)=add(oldy,P(0x10));hit=solid(w,P(4),P(8),1);}
                if(hit){
                    if(P(0x34)==3 || P(0x34)==4)w->explode(w->user,p);
                    else {P(0x18)=P(0x30)>0?1:49;w->sound(w->user,P(4)/1000,P(8)/1000);}
                }
            }
        }
        if(P(0x24)>0){P(0x24)=sub(P(0x24),ticks);if(P(0x24)<=0)w->explode(w->user,p);continue;}
        if(step&3)continue;
        int32_t radius=P(0x34)==4?bits((uint32_t)add(P(0x38),w->proximity_bonus)*16000u):16000;
        int32_t lx=sub(P(4),radius)/64000,ly=sub(P(8),radius)/64000;
        int32_t hx=add(P(4),radius)/64000,hy=add(P(8),radius)/64000;
        if(lx<0)lx=0;
        if(ly<0)ly=0;
        if(hx>255)hx=255;
        if(hy>255)hy=255;
        int occupied=0;
        for(int32_t x=lx;x<=hx && !occupied;++x)
            for(int32_t y=ly;y<=hy;++y)if(w->coverage[y*w->coverage_stride+x/8]&(128u>>(x&7))){occupied=1;break;}
        if(!occupied)continue;
        for(int32_t i=0;i<w->ship_count;++i) {
            SniperShipObject *s=w->ships[i];
            if(s->word[0xa4/4]!=0 || s->word[0x68/4]==P(0x20) ||
                abs32(sub(s->word[0x48/4],P(4)))>=radius || abs32(sub(s->word[0x4c/4],P(8)))>=radius)continue;
            if(P(0x34)==4){P(0x24)=w->proximity_delay;continue;}
            if(P(0x34)==3){w->explode(w->user,p);break;}
            for(int32_t j=0;j<w->projectile_count;++j)
                if(w->projectiles[j]->word[0x1c/4]==P(0x1c))w->projectiles[j]->word[0x18/4]=49;
            if(P(0x30)>0){P(0x18)=1;continue;}
            P(0x18)=49;
            if(s->word[0x44/4]==w->local_id) {
                int32_t owner=P(0),level=add(P(0x38),1);
                int32_t amount=(w->random(w->user)*1000)%w->damage_divisor;
                amount=bits((uint32_t)amount*(uint32_t)level);
                w->damage(w->user,s,amount,owner);break;
            }
        }
        if(P(0x18)>1)w->sound(w->user,P(4)/1000,P(8)/1000);
    }
    if(P(0x18)>0)return 0;
    return P(0x14)>=(P(0x34)==3 || P(0x34)==4 ? w->bomb_lifetime:w->bullet_lifetime);
}
