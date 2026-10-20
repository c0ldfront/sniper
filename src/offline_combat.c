#include "offline_combat.h"
#include <string.h>
#define CFG(a) c->ships->config.word[((a)-0x4281c9)/4]
static int32_t count(SniperOfflineCombat *c){return c->active_update?c->active_update->projectile_count:c->count;}
static void sound(void *context,int32_t handle,int32_t x,int32_t y){
    SniperOfflineCombat *c=context;c->sound(c->user,handle,x,y);
}
static void death_sound(void *context,int32_t x,int32_t y){SniperOfflineCombat *c=context;sound(c,c->death_sound,x,y);}
static void impact_sound(void *context,int32_t x,int32_t y){SniperOfflineCombat *c=context;sound(c,c->impact_sound,x,y);}
static void explosion_sound(void *context,int32_t x,int32_t y){SniperOfflineCombat *c=context;sound(c,c->explosion_sound,x,y);}
static int32_t random_value(void *context){SniperOfflineCombat *c=context;return (int32_t)sniper_random(&c->ships->random_state);}
static void message(void *context,const char *text,int32_t duration,int32_t color){
    SniperOfflineCombat *c=context;sniper_notice_push(&c->ships->attachment_hud,text,duration,color);
}
static void damage(void *context,SniperShipObject *s,int32_t amount,int32_t owner){sniper_offline_combat_damage(context,s,amount,owner);}
static void detach(void *context,int32_t target){SniperOfflineCombat *c=context;sniper_ship_world_offline_attach(c->ships,target);}
static void death(void *context,int32_t killer,int32_t bounty){
    SniperOfflineCombat *c=context;
    /*411540 suppresses prize creation, so the event only needs an empty view. */
    SniperPickupWorld pickups={0};pickups.map=c->ships->movement.map;
    SniperDeathWorld w={c->ships->players,c->ships->players[0],0,CFG(0x42826d),CFG(0x428211),
        &pickups,&c->ships->attachment_hud,NULL,damage,detach,c};
    sniper_offline_death(&w,killer,bounty);
}
void sniper_offline_combat_damage(SniperOfflineCombat *c,SniperShipObject *s,int32_t amount,int32_t owner){
    SniperDamageWorld w={&c->ships->config,c->ships->movement.map,0,random_value,death_sound,death,message,c};
    sniper_ship_damage(s,&w,amount,owner);
}
void sniper_offline_combat_construct(SniperOfflineCombat *c,SniperProjectile *p,const int32_t args[15]){
    SniperProjectileCreateWorld w={.local=c->ships->players[0],.map=c->ships->movement.map,
        .projectiles=c->projectiles,.projectile_count=count(c),.repel_range=CFG(0x4282d1),
        .repel_speed=CFG(0x4282c9),.repel_duration=CFG(0x4282cd),.repel_timer=c->repel_timer,
        .repel_sound=c->repel_sound,.sound=sound,.user=c};
    memcpy(w.bullet_sounds,c->bullet_sounds,sizeof w.bullet_sounds);
    memcpy(w.bomb_sounds,c->bomb_sounds,sizeof w.bomb_sounds);
    sniper_projectile_create(p,&w,args);c->repel_timer=w.repel_timer;
}
static SniperProjectile *create(void *context,const int32_t args[15]){
    SniperOfflineCombat *c=context;SniperProjectile *p=c->allocate(c->user);
    if(p)sniper_offline_combat_construct(c,p,args);
    return p;
}
static void explode(void *context,SniperProjectile *p){
    SniperOfflineCombat *c=context;
    SniperExplosionWorld w={c->ships->players[0],CFG(0x42820d),CFG(0x4281dd),CFG(0x428215),c->shake,
        CFG(0x4281f1),c->next_group,count(c),c->projectiles,explosion_sound,damage,create,c};
    sniper_projectile_explode(p,&w);c->shake=w.shake;c->next_group=w.next_group;c->count=w.projectile_count;
    if(c->active_update)c->active_update->projectile_count=w.projectile_count;
}
static void release(void *context,SniperProjectile *p){SniperOfflineCombat *c=context;c->release(c->user,p);}
static SniperProjectileWorld update_world(SniperOfflineCombat *c){
    SniperProjectileWorld w={c->ships->movement.map,c->ships->coverage,32,c->ordered_ships,c->ships->player_count,
        c->projectiles,c->count,0,CFG(0x4281e1),CFG(0x4281e5),CFG(0x428269),CFG(0x4281e9),CFG(0x4281d1),
        explode,impact_sound,random_value,damage,c};
    return w;
}
void sniper_offline_combat_frame(SniperOfflineCombat *c,const int32_t *ticks){
    SniperProjectileWorld w=update_world(c);
    c->active_update=&w;sniper_projectile_world_frame(&w,ticks,release,c);
    c->count=w.projectile_count;c->active_update=NULL;
}
static int advance_created(void *context,SniperProjectile *p,int32_t ticks){
    SniperOfflineCombat *c=context;
    if(c->active_update)return sniper_projectile_update(p,c->active_update,ticks);
    SniperProjectileWorld w=update_world(c);c->active_update=&w;
    int result=sniper_projectile_update(p,&w,ticks);
    c->count=w.projectile_count;c->active_update=NULL;return result;
}
void sniper_offline_combat_weapon(SniperOfflineCombat *c,const int32_t args[14]){
    SniperWeaponCreateWorld w={c->projectiles,c->active_update?&c->active_update->projectile_count:&c->count,
        &c->next_group,CFG(0x4281ed),CFG(0x428209),create,advance_created,release,c};
    sniper_weapon_create(&w,args);
}

static void action_weapon(void *context,const int32_t args[14]){sniper_offline_combat_weapon(context,args);}
int sniper_offline_combat_action(SniperOfflineCombat *c,int active,int32_t action,int mine){
    SniperWeaponActionWorld w={c->ships->players[0],&c->ships->config,c->ships->movement.map,
        c->projectiles,count(c),0,c->weapon_cooldown,c->safe_timer,c->ships->position_dirty,
        &c->ships->attachment_hud,action_weapon,c};
    int result=sniper_weapon_action(&w,active,action,mine);
    c->weapon_cooldown=w.cooldown;c->safe_timer=w.safe_timer;c->ships->position_dirty=w.position_dirty;
    return result;
}

int sniper_offline_combat_input(SniperOfflineCombat *c,const SniperWeaponInput *input,int32_t ticks){
    SniperWeaponSelection selected=sniper_weapon_input(input,ticks,&c->weapon_cooldown);
    int mine=0;
    if(selected.active && c->weapon_cooldown<=0 && selected.action==1 && c->ships->players[0]->word[0xd4/4]>0)
        mine=(input->key_state(input->user,16)&0x8000) || (input->joystick_buttons&8);
    return sniper_offline_combat_action(c,selected.active,selected.action,mine);
}
