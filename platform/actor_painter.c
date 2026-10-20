#include "actor_painter.h"
typedef struct {
    SniperActorLayers *layers;SniperShipDrawOrder *order;
    const SniperShipPaintOptions *options;SniperOverlayPainter *overlay;
    SniperProjectilePainter *projectiles;SniperShipPainter *ships;
    LPDIRECTDRAWSURFACE target;int suppress;HRESULT result;
} Context;
static void retain(Context *c,HRESULT hr){if(FAILED(hr) && SUCCEEDED(c->result))c->result=hr;}
static void pickup(void *ctx,SniperPickupObject *p,int32_t x,int32_t y){
    Context *c=ctx;retain(c,sniper_pickups_paint(c->overlay,c->target,&p,1,x,y));
}
static void projectile(void *ctx,SniperProjectile *p,int32_t x,int32_t y){
    Context *c=ctx;retain(c,sniper_projectiles_paint(c->projectiles,c->target,&p,1,x,y,c->suppress));
}
static void ships(void *ctx){
    Context *c=ctx;c->order->camera_x=*c->layers->camera_x;c->order->camera_y=*c->layers->camera_y;
    retain(c,sniper_ships_paint(c->ships,c->target,c->order,c->options));
}
HRESULT sniper_actors_paint(SniperActorLayers *layers,SniperShipDrawOrder *order,
    const SniperShipPaintOptions *options,SniperOverlayPainter *overlay,SniperProjectilePainter *projectiles,
    SniperShipPainter *painter,LPDIRECTDRAWSURFACE target,int suppress){
    Context c={layers,order,options,overlay,projectiles,painter,target,suppress,DD_OK};
    SniperActorLayers saved=*layers;
    layers->pickup=pickup;layers->projectile=projectile;layers->ships=ships;layers->user=&c;
    sniper_actor_layers(layers);
    layers->pickup=saved.pickup;layers->projectile=saved.projectile;layers->ships=saved.ships;layers->user=saved.user;
    return c.result;
}
