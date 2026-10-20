#include "ship_damage.h"
#include <string.h>
static int32_t bits(uint32_t x){int32_t r;memcpy(&r,&x,4);return r;}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
#define S(o) s->word[(o)/4]
#define CFG(a) w->config->word[((a)-0x4281c9)/4]
static const char *const messages[16]={
"DAMAGE: Charge rate decreased.","DAMAGE: Maximum energy level decreased.",
"DAMAGE: Rotation speed decreased.","DAMAGE: Stealth lost.","DAMAGE: Cloak lost.",
"DAMAGE: X-Radar lost.","DAMAGE: Guns downgraded.","DAMAGE: Bombs downgraded.",
"DAMAGE: Bouncing bullets lost.","DAMAGE: MultiFire lost.",
"DAMAGE: Thrusters downgraded.","DAMAGE: Top speed reduced.",
"DAMAGE: Proximity bombs lost.","DAMAGE: Shrapnel reduced.",
"DAMAGE: Ghosting lost.","DAMAGE: Repeller lost."};
void sniper_ship_damage(SniperShipObject *s,SniperDamageWorld *w,int32_t amount,int32_t owner){
    if(amount==-1){S(0x5c)=-1;S(0x60)=102;w->sound(w->user,S(0x48)/1000,S(0x4c)/1000);return;}
    if(S(0x5c)<0 || amount<=0)return;
    int32_t x=S(0x48)/16000,y=S(0x4c)/16000;
    if(x>=0 && y>=0 && x<1024 && y<1024 && w->map[y*1024+x]==152)return;
    if(S(0x108)>0)return;
    S(0x5c)=sub(S(0x5c),amount);
    if(S(0x5c)<0){
        S(0x60)=102;w->sound(w->user,S(0x48)/1000,S(0x4c)/1000);
        if(S(0x44)==w->local_id)w->death(w->user,owner,S(0xa0));
        return;
    }
    int32_t divisor=bits((uint32_t)CFG(0x4281d1)*(uint32_t)CFG(0x428291))/amount;
    divisor=bits((uint32_t)divisor+1);
    if(w->random(w->user)%divisor)return;
    int32_t choice=w->random(w->user)%16;
    if(choice<0)return;
    static const unsigned offsets[16]={0xc4,0xc8,0xcc,0xec,0xf0,0xf8,0xd0,0xd4,0xe8,0xfc,0xe0,0xe4,0x100,0xd8,0xf4,0xdc};
    unsigned off=offsets[choice];int32_t value=S(off);
    if(choice<=2 || choice==10 || choice==11){
        static const unsigned bases[5]={0x428251,0x42824d,0x428241,0x428245,0x428249};
        static const unsigned steps[5]={0x428265,0x428261,0x428255,0x428259,0x42825d};
        int i=choice<=2?choice:choice-7;int32_t base=CFG(bases[i]);
        if(value<=base)return;
        value=sub(value,CFG(steps[i]));if(value<base)value=base;S(off)=value;
    }else if(choice==6 || choice==7 || choice==13 || choice==15){
        int32_t floor=choice==6?1:0;if(value<=floor)return;
        value=sub(value,choice==13?CFG(0x428281):1);if(value<floor)value=floor;S(off)=value;
        if(choice==7 && value==0)S(0x9c)=0;
    }else{
        if(!value)return;
        S(off)=0;
        if(choice==3 || choice==4 || choice==5 || choice==14){
            unsigned active=choice==3?0x88:choice==4?0x8c:choice==5?0x94:0x90;
            S(active)=0;S(0x9c)=0;
        }else if(choice==9)S(0x98)=0;
    }
    w->message(w->user,messages[choice],300,2);
    int32_t bounty=sub(S(0xa0),1);S(0xa0)=bounty>0?bounty:0;
    int32_t i=S(0x214)*4;
    s->word[0x154/4+i]=S(0x48)/1000;s->word[0x158/4+i]=S(0x4c)/1000;
    s->word[0x15c/4+i]=52;s->word[0x160/4+i]=1;S(0x214)=(S(0x214)+1)%12;
}
