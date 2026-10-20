#include "projectile_frame.h"
#include <string.h>
void sniper_projectile_frame(SniperProjectile **items,int32_t *count,const int32_t *ticks,
    SniperProjectileStep update,SniperProjectileRelease release,void *user){
    for(int32_t i=0;i<*count;){
        if(update(user,items[i],*ticks)){
            release(user,items[i]);
            --*count;
            memmove(items+i,items+i+1,(size_t)(*count-i)*sizeof *items);
        }else ++i;
    }
}
typedef struct {SniperProjectileWorld *world;SniperProjectileRelease release;void *user;} FrameContext;
static int update_world(void *context,SniperProjectile *p,int32_t ticks){
    FrameContext *c=context;return sniper_projectile_update(p,c->world,ticks);
}
static void release_world(void *context,SniperProjectile *p){
    FrameContext *c=context;c->release(c->user,p);
}
void sniper_projectile_world_frame(SniperProjectileWorld *w,const int32_t *ticks,
    SniperProjectileRelease release,void *user){
    FrameContext context={w,release,user};
    sniper_projectile_frame(w->projectiles,&w->projectile_count,ticks,update_world,release_world,&context);
}
