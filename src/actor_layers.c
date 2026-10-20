#include "actor_layers.h"
void sniper_actor_layers(SniperActorLayers *w){
    for(int32_t i=0;i<*w->pickup_count;++i)w->pickup(w->user,w->pickups[i],*w->camera_x,*w->camera_y);
    for(int32_t i=0;i<*w->projectile_count;++i){
        SniperProjectile *p=w->projectiles[i];
        if(p->word[0x18/4]<=0)w->projectile(w->user,p,*w->camera_x,*w->camera_y);
    }
    w->ships(w->user);
    for(int32_t i=0;i<*w->projectile_count;++i){
        SniperProjectile *p=w->projectiles[i];
        if(p->word[0x18/4]>0)w->projectile(w->user,p,*w->camera_x,*w->camera_y);
    }
}
