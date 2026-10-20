#include "ship_update.h"
#include "ship_energy.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s; memcpy(&s,&u,4); return s; }
static int32_t sub(int32_t a,int32_t b) { return bits((uint32_t)a-(uint32_t)b); }
static int32_t add(int32_t a,int32_t b) { return bits((uint32_t)a+(uint32_t)b); }
#define S(o) (s->word[(o)/4])
void sniper_ship_update(SniperShipObject *s,SniperShipUpdateWorld *w,int32_t dt)
{
    S(0xa4)=0;
    if (S(0x44)!=w->local_id) {
        if (S(0x48)==-1) S(0xa4)=2;
        else {
            int32_t age=bits(w->milliseconds/10u-(uint32_t)S(0x74));
            if (age>500) S(0xa4)=2;
            else if (age>300) S(0xa4)=1;
        }
        if (S(0xa4)==2) { S(0x60)=0; return; }
    }
    for (unsigned o=0x15c;o<=0x20c;o+=16)
        if (S(o)>0) S(o)=sub(S(o),dt);
    if (S(0xbc)>0) S(0xbc)=sub(S(0xbc),dt);
    if (S(0x64)>0) {
        S(0xa4)=2; S(0x64)=sub(S(0x64),dt);
        if (S(0x64)<=0) w->spawn(w->user,s,1,1);
        return;
    }
    if (S(0x60)>0) {
        S(0x48)=add(S(0x48),S(0x50));
        S(0x4c)=add(S(0x4c),S(0x54));
        S(0x60)=sub(S(0x60),dt);
        if (S(0x60)>0) return;
        S(0xa4)=2;
        if (S(0x44)==w->local_id) S(0x64)=w->respawn_delay;
        else { S(0x48)=-1; S(0x4c)=-1; S(0x5c)=w->remote_energy; }
        return;
    }
    if (S(0x104)>0) S(0x104)=sub(S(0x104),dt);
    if (S(0x108)>0) S(0x108)=sub(S(0x108),dt);
    if (S(0x10c)>0) {
        S(0x10c)=sub(S(0x10c),dt);
        if (S(0x10c)<=0) {
            static const unsigned dest[]={0xc4,0xc8,0xcc,0xe0,0xe4,0xd0,
                0xd8,0xd4,0xe8,0xfc,0xec,0xf0,0xf4,0xf8,0x100};
            S(0x9c)=0; S(0x8c)=0;
            for (unsigned i=0;i<15;++i) S(dest[i])=S(0x110+4*i);
            S(0x5c)=S(0x150);
            S(0x90)=0; S(0x88)=0; S(0x94)=0; S(0x98)=S(0x14c);
        }
    }
    if (S(0x90)!=0 && S(0x68)!=w->local_team) {
        S(0xb0)=sub(S(0xb0),dt);
        if (S(0xb0)<=0) {
            S(0xb0)=w->random(w->user)%w->jitter_period;
            S(0xb4)=sub(w->random(w->user)%add(w->jitter_radius,w->jitter_radius),w->jitter_radius);
            S(0xb8)=sub(w->random(w->user)%add(w->jitter_radius,w->jitter_radius),w->jitter_radius);
            S(0xbc)=48;
        }
    } else { S(0xb4)=0; S(0xb8)=0; }
    SniperShipObject *parent=0;
    if (S(0x6c)!=-1) {
        parent=w->lookup(w->user,S(0x6c));
        if (!parent) w->detach(w->user,-1);
    }
    if (parent) {
        if (parent->word[0x48/4]>0 && parent->word[0x4c/4]>0)
            for (unsigned o=0x48;o<=0x54;o+=4) S(o)=parent->word[o/4];
    } else {
        if (S(0x48)==-1) return;
        w->move(w->user,s,dt,1);
    }
    int32_t x=S(0x48)/64000,y=S(0x4c)/64000;
    if (x>=0 && y>=0 && x<256 && y<256)
        w->coverage[y*w->coverage_stride+x/8]|=(uint8_t)(128u>>(x&7));
    if (S(0x44)==w->local_id) {
        SniperShipEnergy energy={S(0x5c),S(0xc4),S(0xc8),{S(0x88),S(0x8c),S(0x90),S(0x94)}};
        sniper_ship_energy_step(&energy,dt,w->ability_cost);
        S(0x5c)=energy.energy;
        for(unsigned i=0;i<4;++i) S(0x88+4*i)=energy.ability[i];
    }
}
