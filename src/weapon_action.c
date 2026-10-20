#include "weapon_action.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return bits((uint32_t)a*(uint32_t)b);}
#define S(o) w->local->word[(o)/4]
#define CFG(a) w->config->word[((a)-0x4281c9)/4]
int sniper_weapon_action_fields(SniperWeaponActionWorld *w,int active,int32_t action,int mine,int32_t fields[6]){
    int32_t type=0,cost=0,level=0,fragment_type=0,fragment_level=0,fragments=0,multi=0;
    if(fields){level=fields[1];fragment_type=fields[2];fragment_level=fields[3];fragments=fields[4];multi=fields[5];}
    if(active && w->cooldown<=0){
        if(action==0 && S(0xd0)>0){
            w->cooldown=S(0x98)?CFG(0x4281f9):CFG(0x4281f5);
            cost=mul(S(0xd0),S(0x98)?CFG(0x4281cd):CFG(0x4281c9));
            fragment_type=fragment_level=fragments=0;type=S(0xe8)==0?1:2;level=sub(S(0xd0),1);multi=S(0x98)?1:0;
        }else if(action==1 && S(0xd4)>0){
            w->cooldown=CFG(0x4281fd);cost=mul(CFG(0x4281d5),S(0xd4));
            type=S(0x100)==0?3:4;level=sub(S(0xd4),1);
            multi=0;fragment_type=S(0xe8)==0?1:2;fragment_level=sub(S(0xd0),1);fragments=S(0xd8);
            if(mine){
                int32_t n=0;
                for(int32_t i=0;i<w->count;++i){SniperProjectile *p=w->projectiles[i];
                    if(p->word[8]==S(0x68) && !p->word[3] && !p->word[4])++n;
                }
                if(n<CFG(0x4282b5)){multi=1;cost=mul(S(0xd4),CFG(0x4281d9));w->cooldown=CFG(0x428201);}
                else {sniper_notice_push(w->notices,"Team has too many mines",400,2);type=0;}
            }
        }else if(action>=2 && action<=6){
            unsigned available=0xec+(unsigned)(action-2)*4,enabled=0x88+(unsigned)(action-2)*4;
            if(S(available)){w->cooldown=CFG(0x4281f5);S(enabled)=S(enabled)==0;
                if(action==3 && !S(enabled))w->position_dirty=1;
            }
        }else if(action==7 && S(0xdc)>0){w->cooldown=CFG(0x4281fd);type=5;level=fragment_type=fragment_level=fragments=multi=0;}
    }
    if(S(0x5c)<cost)type=0;
    int32_t x=S(0x48)/16000,y=S(0x4c)/16000;
    int safe=x>=0 && y>=0 && x<1024 && y<1024 && w->map[y*1024+x]==152;
    if(safe){if(type){S(0x50)=0;S(0x54)=0;}type=0;if(w->safe_timer<=0)w->safe_timer=CFG(0x428295);}
    else w->safe_timer=0;
    if(type){
        if(type==5){int32_t n=sub(S(0xdc),1);S(0xdc)=n>0?n:0;}
        if(S(0x8c)){w->position_dirty=1;S(0x8c)=0;}
        int32_t energy=sub(S(0x5c),cost);S(0x5c)=energy>0?energy:0;
        int32_t args[14]={type,level,fragment_type,fragment_level,fragments,multi,0,
            mul(S(0x58)/1000,1000),S(0x48),S(0x4c),S(0x50),S(0x54),w->local_id,S(0x68)};
        w->weapon(w->user,args);
    }
    if(fields){int32_t output[6]={type,level,fragment_type,fragment_level,fragments,multi};memcpy(fields,output,sizeof output);}
    return type;
}

int sniper_weapon_action(SniperWeaponActionWorld *w,int active,int32_t action,int mine)
{return sniper_weapon_action_fields(w,active,action,mine,NULL);}
