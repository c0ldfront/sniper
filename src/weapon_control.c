#include "weapon_control.h"
int sniper_weapon_action_dispatch(SniperWeaponActionWorld *w,SniperPositionDispatch *p,int active,int32_t action,int mine,int32_t fields[6])
{
    int32_t local[6]={0};if(!fields)fields=local;
    sniper_weapon_action_fields(w,active,action,mine,fields);
    p->local=w->local;p->position_dirty=w->position_dirty;
    int sent=sniper_position_dispatch(p,fields);w->position_dirty=p->position_dirty;
    return sent;
}
int sniper_weapon_control_fields(SniperWeaponActionWorld *w,const SniperWeaponInput *input,int32_t ticks,int32_t fields[6])
{
    SniperWeaponSelection selected=sniper_weapon_input(input,ticks,&w->cooldown);
    int mine=0;
    if(selected.active&&w->cooldown<=0&&selected.action==1&&w->local->word[0xd4/4]>0)
        mine=(input->key_state(input->user,16)&0x8000)||(input->joystick_buttons&8);
    return sniper_weapon_action_fields(w,selected.active,selected.action,mine,fields);
}

int sniper_weapon_control(SniperWeaponActionWorld *w,SniperPositionDispatch *p,const SniperWeaponInput *input,int32_t ticks,int32_t fields[6])
{
    int32_t local[6]={0};if(!fields)fields=local;
    sniper_weapon_control_fields(w,input,ticks,fields);
    p->local=w->local;p->position_dirty=w->position_dirty;
    int sent=sniper_position_dispatch(p,fields);w->position_dirty=p->position_dirty;
    return sent;
}
