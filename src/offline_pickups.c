#include "offline_pickups.h"
static void sync_view(SniperOfflinePickups *p) {
    for(int32_t i=0;i<p->collection.count;++i) {
        SniperPickupObject *item=p->collection.items[i];
        p->movement_view[i]=(SniperPickup){item->x,item->y,item->value};
    }
    p->ships->movement.pickups=p->movement_view;
    p->ships->movement.pickup_count=p->collection.count;
}
static SniperShipObject *lookup(void *context,int32_t id) {
    SniperOfflinePickups *p=context;
    return id>=0 && id<1024 ? p->ships->players[id] : 0;
}
static void release(void *context,SniperPickupObject *item) {
    SniperOfflinePickups *p=context;p->release(p->owner,item);
}
static void award(void *context,SniperShipObject *ship,int32_t code) {
    SniperOfflinePickups *p=context;
    sync_view(p);
    sniper_ship_world_prize(p->ships,ship,code);
}
void sniper_offline_pickups_collect(SniperOfflinePickups *p,int32_t x,int32_t y,
    int32_t collector,int32_t code) {
    SniperShipWorld *w=p->ships;
    p->collection.map=w->movement.map;
    p->collection.local=w->players[w->movement.local_id];
    p->collection.sharing_enabled=w->config.word[(0x4282ad-0x4281c9)/4];
    p->collection.sharing_limit=w->config.word[(0x4282b9-0x4281c9)/4];
    sniper_pickup_collect(&p->collection,x,y,collector,code);
    sync_view(p);
}
static void pickup(void *context,int32_t x,int32_t y,int32_t player,int32_t value,int local) {
    SniperOfflinePickups *p=context;
    sniper_offline_pickups_collect(p,x,y,local ? 0:player,local ? value:0);
}
static void impact(void *context,int32_t x,int32_t y) {
    SniperOfflinePickups *p=context;
    if(p->impact_sound)p->impact_sound(p->impact_user,x,y);
}
void sniper_offline_pickups_bind(SniperOfflinePickups *p) {
    SniperMoveWorld *m=&p->ships->movement;
    p->impact_sound=m->impact_sound;p->impact_user=m->user;
    p->collection.lookup=lookup;p->collection.release=release;
    p->collection.award=award;p->collection.user=p;
    m->pickup=pickup;m->impact_sound=impact;m->user=p;
    sync_view(p);
}

static uint32_t clock_ms(void *context) {
    SniperOfflinePickups *p=context;return p->clock_ms(p->clock_user);
}
static void create(void *context,int32_t x,int32_t y,int32_t code,int32_t lifetime) {
    SniperOfflinePickups *p=context;
    p->collection.map=p->ships->movement.map;
    sniper_pickup_create(&p->collection,x,y,code,lifetime,p->allocate,p->owner);
    sync_view(p);
}
void sniper_offline_pickups_generate(SniperOfflinePickups *p) {
    sniper_offline_generate(&p->generation_tick,&p->ships->random_state,
        p->ships->movement.map,clock_ms,create,p);
}
void sniper_offline_pickups_expire(SniperOfflinePickups *p,int32_t ticks) {
    p->collection.map=p->ships->movement.map;
    sniper_pickup_expire(&p->collection,ticks);sync_view(p);
}
