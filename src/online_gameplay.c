#include "online_gameplay.h"
#include <string.h>
#define CFG(a) c->ships->config.word[((a)-0x4281c9)/4]
static SniperShipObject *local_player(SniperOnlineGameplay *g){
    return g->local_id>=0&&g->local_id<255?g->combat->ships->players[g->local_id]:NULL;
}
static int32_t count(SniperOfflineCombat *c){return c->active_update?c->active_update->projectile_count:c->count;}
static void sound(void *context,int32_t handle,int32_t x,int32_t y){
    SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;c->sound(c->user,handle,x,y);
}
static void death_sound(void *context,int32_t x,int32_t y){SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;sound(g,c->death_sound,x,y);}
static void impact_sound(void *context,int32_t x,int32_t y){SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;sound(g,c->impact_sound,x,y);}
static void explosion_sound(void *context,int32_t x,int32_t y){SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;sound(g,c->explosion_sound,x,y);}
static int32_t random_value(void *context){SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;return (int32_t)sniper_random(&c->ships->random_state);}
static void message(void *context,const char *text,int32_t duration,int32_t color){
    SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;sniper_notice_push(&c->ships->attachment_hud,text,duration,color);
}
static void damage(void *context,SniperShipObject *s,int32_t amount,int32_t owner){sniper_online_gameplay_damage(context,s,amount,owner);}
static void death(void *context,int32_t killer,int32_t bounty){
    SniperOnlineGameplay *g=context;sniper_network_client_death(g->client,killer,bounty);
}
void sniper_online_gameplay_damage(SniperOnlineGameplay *g,SniperShipObject *s,int32_t amount,int32_t owner){
    SniperOfflineCombat *c=g->combat;
    SniperDamageWorld w={&c->ships->config,c->ships->movement.map,g->local_id,random_value,death_sound,death,message,g};
    sniper_ship_damage(s,&w,amount,owner);
}
void sniper_online_gameplay_construct(SniperOnlineGameplay *g,SniperProjectile *p,const int32_t args[15]){
    SniperOfflineCombat *c=g->combat;
    SniperProjectileCreateWorld w={.local=c->ships->players[g->local_id],.map=c->ships->movement.map,
        .projectiles=c->projectiles,.projectile_count=count(c),.repel_range=CFG(0x4282d1),
        .repel_speed=CFG(0x4282c9),.repel_duration=CFG(0x4282cd),.repel_timer=c->repel_timer,
        .repel_sound=c->repel_sound,.sound=sound,.user=g};
    memcpy(w.bullet_sounds,c->bullet_sounds,sizeof w.bullet_sounds);
    memcpy(w.bomb_sounds,c->bomb_sounds,sizeof w.bomb_sounds);
    sniper_projectile_create(p,&w,args);c->repel_timer=w.repel_timer;
}
static SniperProjectile *create(void *context,const int32_t args[15]){
    SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;SniperProjectile *p=c->allocate(c->user);
    if(p)sniper_online_gameplay_construct(g,p,args);
    return p;
}
static void explode(void *context,SniperProjectile *p){
    SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;
    SniperExplosionWorld w={c->ships->players[g->local_id],CFG(0x42820d),CFG(0x4281dd),CFG(0x428215),c->shake,
        CFG(0x4281f1),c->next_group,count(c),c->projectiles,explosion_sound,damage,create,g};
    sniper_projectile_explode(p,&w);c->shake=w.shake;c->next_group=w.next_group;c->count=w.projectile_count;
    if(c->active_update)c->active_update->projectile_count=w.projectile_count;
}
static void release(void *context,SniperProjectile *p){SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;c->release(c->user,p);}
static SniperProjectileWorld update_world(SniperOnlineGameplay *g){
    SniperOfflineCombat *c=g->combat;
    SniperProjectileWorld w={c->ships->movement.map,c->ships->coverage,32,c->ordered_ships,c->ships->player_count,
        c->projectiles,c->count,g->local_id,CFG(0x4281e1),CFG(0x4281e5),CFG(0x428269),CFG(0x4281e9),CFG(0x4281d1),
        explode,impact_sound,random_value,damage,g};
    return w;
}
void sniper_online_gameplay_combat_frame(SniperOnlineGameplay *g,const int32_t *ticks){
    SniperOfflineCombat *c=g->combat;
    SniperProjectileWorld w=update_world(g);
    c->active_update=&w;sniper_projectile_world_frame(&w,ticks,release,g);
    c->count=w.projectile_count;c->active_update=NULL;
}
static int advance_created(void *context,SniperProjectile *p,int32_t ticks){
    SniperOnlineGameplay *g=context;SniperOfflineCombat *c=g->combat;
    if(c->active_update)return sniper_projectile_update(p,c->active_update,ticks);
    SniperProjectileWorld w=update_world(g);c->active_update=&w;
    int result=sniper_projectile_update(p,&w,ticks);
    c->count=w.projectile_count;c->active_update=NULL;return result;
}
void sniper_online_gameplay_weapon(SniperOnlineGameplay *g,const int32_t args[14]){
    SniperOfflineCombat *c=g->combat;
    SniperWeaponCreateWorld w={c->projectiles,c->active_update?&c->active_update->projectile_count:&c->count,
        &c->next_group,CFG(0x4281ed),CFG(0x428209),create,advance_created,release,g};
    sniper_weapon_create(&w,args);
}

static void action_weapon(void *context,const int32_t args[14]){sniper_online_gameplay_weapon(context,args);}
int sniper_online_gameplay_action(SniperOnlineGameplay *g,int active,int32_t action,int mine){
    SniperOfflineCombat *c=g->combat;
    SniperWeaponActionWorld w={c->ships->players[g->local_id],&c->ships->config,c->ships->movement.map,
        c->projectiles,count(c),g->local_id,c->weapon_cooldown,c->safe_timer,c->ships->position_dirty,
        &c->ships->attachment_hud,action_weapon,g};
    int result=sniper_weapon_action(&w,active,action,mine);
    c->weapon_cooldown=w.cooldown;c->safe_timer=w.safe_timer;c->ships->position_dirty=w.position_dirty;
    return result;
}

int sniper_online_gameplay_input(SniperOnlineGameplay *g,const SniperWeaponInput *input,int32_t ticks){
    SniperOfflineCombat *c=g->combat;
    SniperWeaponSelection selected=sniper_weapon_input(input,ticks,&c->weapon_cooldown);
    int mine=0;
    if(selected.active && c->weapon_cooldown<=0 && selected.action==1 && c->ships->players[g->local_id]->word[0xd4/4]>0)
        mine=(input->key_state(input->user,16)&0x8000) || (input->joystick_buttons&8);
    return sniper_online_gameplay_action(g,selected.active,selected.action,mine);
}

typedef struct {SniperOnlineGameplay *game;SniperLocalControl original;} ControlContext;
static void control_damage(void*u,SniperShipObject*s,int32_t amount,int32_t owner){ControlContext*x=u;sniper_online_gameplay_damage(x->game,s,amount,owner);}
static uint32_t control_poll(void*u,uint32_t id,uint32_t info[13]){ControlContext*x=u;return x->original.poll_joystick(x->original.user,id,info);}
static void control_send(void*u,const int32_t args[13]){SniperOnlineGameplay*g=u;sniper_network_client_position(g->client,args);}
int sniper_online_gameplay_control(SniperOnlineGameplay*g,SniperOrdinaryControl*control){
 SniperOfflineCombat*c=g->combat;
 SniperWeaponActionWorld weapons={c->ships->players[g->local_id],&c->ships->config,c->ships->movement.map,c->projectiles,count(c),g->local_id,c->weapon_cooldown,c->safe_timer,c->ships->position_dirty,&c->ships->attachment_hud,action_weapon,g};
 ControlContext context={g,*control->lifecycle};SniperWeaponActionWorld*original_weapons=control->weapons;
 SniperPositionDispatch*p=control->position;void(*original_send)(void*,const int32_t*)=p->send;void*original_user=p->user;
 control->weapons=&weapons;control->lifecycle->local=c->ships->players[g->local_id];control->lifecycle->local_id=g->local_id;
 control->lifecycle->damage=control_damage;control->lifecycle->poll_joystick=control_poll;control->lifecycle->user=&context;p->send=control_send;p->user=g;
 int result=sniper_ordinary_control(control);
 c->weapon_cooldown=weapons.cooldown;c->safe_timer=weapons.safe_timer;c->ships->position_dirty=weapons.position_dirty;
 p->send=original_send;p->user=original_user;control->lifecycle->damage=context.original.damage;control->lifecycle->poll_joystick=context.original.poll_joystick;control->lifecycle->user=context.original.user;control->weapons=original_weapons;
 return result;
}
void sniper_online_gameplay_sync_pickups(SniperOnlineGameplay*g){
 SniperOfflinePickups*p=g->pickups;SniperOfflineCombat*c=g->combat;
 p->collection.map=c->ships->movement.map;p->collection.local=local_player(g);p->collection.sharing_enabled=CFG(0x4282ad);p->collection.sharing_limit=CFG(0x4282b9);
 for(int32_t i=0;i<p->collection.count;++i){SniperPickupObject*item=p->collection.items[i];p->movement_view[i]=(SniperPickup){item->x,item->y,item->value};}
 p->ships->movement.pickups=p->movement_view;p->ships->movement.pickup_count=p->collection.count;
}
static SniperShipObject*pickup_lookup(void*u,int32_t id){SniperOnlineGameplay*g=u;return id>=0&&id<255?g->combat->ships->players[id]:NULL;}
static void pickup_release(void*u,SniperPickupObject*item){SniperOnlineGameplay*g=u;SniperOfflinePickups*p=g->pickups;p->release(p->owner,item);}
static void pickup_award(void*u,SniperShipObject*ship,int32_t code){SniperOnlineGameplay*g=u;sniper_online_gameplay_sync_pickups(g);sniper_ship_world_prize(g->combat->ships,ship,code);}
void sniper_online_gameplay_collect(SniperOnlineGameplay*g,int32_t x,int32_t y,int32_t player,int32_t code){
 SniperOfflineCombat*c=g->combat;SniperPickupWorld*w=&g->pickups->collection;
 w->map=c->ships->movement.map;w->local=local_player(g);w->sharing_enabled=CFG(0x4282ad);w->sharing_limit=CFG(0x4282b9);
 if(code&&!w->local)return;
 sniper_pickup_collect(w,x,y,player,code);sniper_online_gameplay_sync_pickups(g);
}
static void movement_pickup(void*u,int32_t x,int32_t y,int32_t player,int32_t value,int local){
 SniperOnlineGameplay*g=u;SniperOfflineCombat*c=g->combat;
 if(local)sniper_network_client_pickup(g->client,x,y,value,(uint32_t)CFG(0x4282dd));
 else sniper_online_gameplay_collect(g,x,y,player,0);
 /* Host's local echo may use backend's collection adapter directly. */
 sniper_online_gameplay_sync_pickups(g);
}
static void movement_impact(void*u,int32_t x,int32_t y){SniperOnlineGameplay*g=u;SniperOfflinePickups*p=g->pickups;if(p->impact_sound)p->impact_sound(p->impact_user,x,y);}
void sniper_online_gameplay_pickups_bind(SniperOnlineGameplay*g){
 SniperOfflinePickups*p=g->pickups;SniperMoveWorld*m=&g->combat->ships->movement;
 p->ships=g->combat->ships;p->impact_sound=m->impact_sound;p->impact_user=m->user;
 p->collection.lookup=pickup_lookup;p->collection.release=pickup_release;p->collection.award=pickup_award;p->collection.user=g;
 p->collection.map=m->map;p->collection.local=local_player(g);
 m->local_id=g->local_id;m->pickup=movement_pickup;m->impact_sound=movement_impact;m->user=g;sniper_online_gameplay_sync_pickups(g);
}
static int32_t subtract(int32_t a,int32_t b){uint32_t u=(uint32_t)a-(uint32_t)b;int32_t s;memcpy(&s,&u,4);return s;}
void sniper_online_gameplay_simulation(SniperOnlineGameplay*g,const int32_t*ticks){
 SniperOfflineCombat*c=g->combat;memset(c->ships->coverage,0,sizeof c->ships->coverage);
 if(g->pickups){g->pickups->collection.map=c->ships->movement.map;sniper_pickup_expire(&g->pickups->collection,*ticks);sniper_online_gameplay_sync_pickups(g);}
 sniper_ship_world_update_all(c->ships,c->ordered_ships,ticks);sniper_online_gameplay_combat_frame(g,ticks);
 sniper_notices_step(&c->ships->attachment_hud,*ticks);if(c->shake>0)c->shake=subtract(c->shake,*ticks);if(c->repel_timer>0)c->repel_timer=subtract(c->repel_timer,*ticks);
}
