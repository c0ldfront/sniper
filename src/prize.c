#include "prize.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s;memcpy(&s,&u,4);return s; }
static int32_t add(int32_t a,int32_t b) {return bits((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b) {return bits((uint32_t)a-(uint32_t)b);}
#define S(o) s->word[(o)/4]
#define CFG(a) w->config->word[((a)-0x4281c9)/4]
static const unsigned stat[6]={0xc4,0xc8,0xcc,0xe0,0xe4,0xd8};
static const unsigned base[5]={0x428251,0x42824d,0x428241,0x428245,0x428249};
static const unsigned bonus[6]={0x428265,0x428261,0x428255,0x428259,0x42825d,0x428281};
static const unsigned limit[6]={0x42823d,0x428239,0x42822d,0x428231,0x428235,0x42827d};
static const unsigned saved[17]={0xc4,0xc8,0xcc,0xe0,0xe4,0xd0,0xd8,0xd4,
    0xe8,0xfc,0xec,0xf0,0xf4,0xf8,0x100,0x98,0x5c};
static const char *const positive[21]={
"Charge rate increased.",
"Maximum energy level increased.",
"Rotation speed increased.",
"Stealth available.",
"Cloak available.",
"X-Radar available.",
"Warp!",
"Guns upgraded.",
"Bombs upgraded.",
"Bouncing bullets.",
"Thrusters upgraded.",
"Top speed increased.",
"Full charge.",
"Sticky glue.",
"MultiFire bullets.",
"Proximity bombs.",
"Temporary SuperPower!",
"Temporary Shields",
"Shrapnel increased.",
"Ghosting available.",
"Repeller increased."
};
static const char *const negative[21]={
"Charge rate decreased.",
"Maximum energy level decreased.",
"Rotation speed decreased.",
"Stealth lost.",
"Cloak lost.",
"X-Radar lost.",
0,
"Guns downgraded.",
"Bombs downgraded.",
"Bouncing bullets lost.",
"Thrusters downgraded.",
"Top speed reduced.",
"Energy depleted.",
"Extra sticky glue.",
"MultiFire lost.",
"Proximity bombs lost.",
0,
0,
"Shrapnel reduced.",
"Ghosting lost.",
"Repeller lost."
};
/* Map stat prizes to their common parameter operations. */
static int stat_index(int32_t code) {
    switch(code) {case 1:return 0;case 2:return 1;case 3:return 2;
    case 11:return 3;case 12:return 4;case 19:return 5;default:return -1;}
}
int sniper_prize(SniperShipObject *s,SniperPrizeWorld *w,int32_t code) {
    if(code>21)return -1;
    const char *text=0;int32_t sound=0;int maxed=0;
    if(code<0) {
        sound=w->negative_sound;
        uint32_t magnitude=0u-(uint32_t)code;
        if(magnitude<=21) {
            int n=(int)magnitude;int idx=stat_index(n);int flip=0;
            text=negative[n-1];
            if(idx>=0) {
                int32_t floor=idx==5 ? 0 : CFG(base[idx]);
                if(S(stat[idx])==floor) flip=1;
                else { S(stat[idx])=sub(S(stat[idx]),CFG(bonus[idx]));
                    if(S(stat[idx])<floor) S(stat[idx])=floor; }
            } else {
                unsigned field=0,ability=0;
                switch(n) {
                case 4:field=0xec;ability=0x88;break;
                case 5:field=0xf0;ability=0x8c;break;
                case 6:field=0xf8;ability=0x94;break;
                case 10:field=0xe8;break;
                case 15:field=0xfc;ability=0x98;break;
                case 16:field=0x100;break;
                case 20:field=0xf4;ability=0x90;break;
                case 7:case 17:case 18:flip=1;break;
                case 8:
                    if(S(0xd0)==1)flip=1;
                    else {S(0xd0)=sub(S(0xd0),1);if(S(0xd0)<1)S(0xd0)=1;}
                    break;
                case 9:
                    if(!S(0xd4))flip=1;
                    else {S(0xd4)=sub(S(0xd4),1);if(S(0xd4)<0)S(0xd4)=0;
                        if(!S(0xd4))S(0x9c)=0;}
                    break;
                case 13:S(0x5c)=1;break;
                case 14:S(0x104)=bits((uint32_t)CFG(0x42821d)*3u);S(0x50)=S(0x54)=0;break;
                case 21:if(!S(0xdc))flip=1;else S(0xdc)=sub(S(0xdc),1);break;
                }
                if(field) {
                    if(!S(field))flip=1;
                    else {S(field)=0;if(ability){S(ability)=0;S(0x9c)=0;}}
                }
            }
            if(flip){code=n;text=0;}
        }
    }
    if(code>0) {
        sound=w->positive_sound;text=positive[code-1];
        int idx=stat_index(code);
        if(idx>=0) {
            int32_t cap=CFG(limit[idx]);
            if((idx==2 || idx==3) && S(0x40)==0)cap=add(cap,CFG(bonus[idx]));
            if(idx==4 && S(0x40)==1)cap=add(cap,CFG(bonus[idx]));
            S(stat[idx])=add(S(stat[idx]),CFG(bonus[idx]));
            if(S(stat[idx])>=cap) {S(stat[idx])=cap;if(idx!=5)maxed=1;}
        } else switch(code) {
        case 4:S(0xec)=1;break;
        case 5:if(S(0x40)==2)S(0xf0)=1;else {S(0xf4)=1;text=positive[19];}break;
        case 6:S(0xf8)=1;break;
        case 7:sound=w->warp_sound;w->spawn(w->user,s,0,1);break;
        case 8:S(0xd0)=add(S(0xd0),1);if(S(0xd0)>3)S(0xd0)=3;break;
        case 9:{int cap=S(0x40)==3 ? 3:2;S(0xd4)=add(S(0xd4),1);if(S(0xd4)>cap)S(0xd4)=cap;break;}
        case 10:S(0xe8)=1;break;
        case 13:S(0x5c)=S(0xc8);break;
        case 14:S(0x104)=CFG(0x42821d);S(0x50)=S(0x54)=0;break;
        case 15:if(!S(0xfc)){S(0xfc)=1;S(0x98)=1;}break;
        case 16:S(0x100)=1;break;
        case 17:
            if(S(0x10c)<=0) {
                S(0x10c)=(w->random(w->user)*100)%CFG(0x428289)+1;
                for(unsigned i=0;i<17;++i)S(0x110+i*4)=S(saved[i]);
                int32_t old_multi=S(0xfc);
                for(unsigned i=0;i<6;++i)S(stat[i])=CFG(limit[i]);
                S(0xd0)=3;S(0xd4)=2;S(0xf0)=0;
                S(0xe8)=S(0xfc)=S(0xec)=S(0xf4)=S(0xf8)=S(0x100)=1;
                switch(S(0x40)) {
                case 0:S(0xe0)=add(S(0xe0),CFG(0x428259));S(0xcc)=add(S(0xcc),CFG(0x428255));break;
                case 1:S(0xe4)=add(S(0xe4),CFG(0x42825d));break;
                case 2:S(0xf0)=1;break;
                case 3:S(0xd4)=3;break;
                }
                if(!old_multi)S(0x98)=1;
                S(0x5c)=S(0xc8);
            }break;
        case 18:if(S(0x108)<=0)S(0x108)=(w->random(w->user)*100)%CFG(0x42828d)+1;break;
        case 20:S(0xf4)=1;break;
        case 21:S(0xdc)=add(S(0xdc),1);if(S(0xdc)>=CFG(0x4282d5))S(0xdc)=CFG(0x4282d5);break;
        }
        S(0xa0)=add(S(0xa0),1);
    }
    w->sound(w->user,sound);
    if(text) {
        char message[128];size_t len=strlen(text);memcpy(message,text,len+1);
        if(maxed)memcpy(message+len," MAX",5);
        w->message(w->user,message,300,3);
    }
    return 0;
}
