#include "offline_control.h"
typedef struct {
    SniperOfflineCombat *combat;
    SniperLocalControl original;
} Context;
static void damage(void *user,SniperShipObject *ship,int32_t amount,int32_t owner) {
    Context *c=user;sniper_offline_combat_damage(c->combat,ship,amount,owner);
}
static uint32_t poll(void *user,uint32_t id,uint32_t info[13]) {
    Context *c=user;return c->original.poll_joystick(c->original.user,id,info);
}
static void weapon(void *user,const int32_t args[14]) {
    sniper_offline_combat_weapon(user,args);
}
static void send(void *user,const int32_t args[13]) {(void)user;(void)args;}
int sniper_offline_control(SniperOfflineCombat *c,SniperOrdinaryControl *control) {
    SniperWeaponActionWorld weapons={c->ships->players[0],&c->ships->config,
        c->ships->movement.map,c->projectiles,c->count,0,c->weapon_cooldown,
        c->safe_timer,c->ships->position_dirty,&c->ships->attachment_hud,weapon,c};
    Context context={c,*control->lifecycle};
    SniperWeaponActionWorld *original_weapons=control->weapons;
    SniperPositionDispatch *p=control->position;
    void (*original_send)(void *,const int32_t *)=p->send;
    void *original_user=p->user;
    control->weapons=&weapons;
    control->lifecycle->local=c->ships->players[0];control->lifecycle->local_id=0;
    control->lifecycle->damage=damage;control->lifecycle->poll_joystick=poll;
    control->lifecycle->user=&context;
    p->send=send;p->user=0;
    int result=sniper_ordinary_control(control);
    c->weapon_cooldown=weapons.cooldown;c->safe_timer=weapons.safe_timer;
    c->ships->position_dirty=weapons.position_dirty;
    p->send=original_send;p->user=original_user;
    control->lifecycle->damage=context.original.damage;
    control->lifecycle->poll_joystick=context.original.poll_joystick;
    control->lifecycle->user=context.original.user;
    control->weapons=original_weapons;
    return result;
}
