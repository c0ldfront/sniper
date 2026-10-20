#include "movement.h"
#include <string.h>
static int32_t bits(uint32_t u) { int32_t s; memcpy(&s,&u,4); return s; }
static int32_t add(int32_t a,int32_t b) { return bits((uint32_t)a+(uint32_t)b); }
static int32_t sub(int32_t a,int32_t b) { return bits((uint32_t)a-(uint32_t)b); }
static int32_t neg(int32_t a) { return bits(0u-(uint32_t)a); }
static int32_t abs32(int32_t a) { return a < 0 ? neg(a) : a; }
static int32_t damp(int32_t v,int32_t divisor) {
    return bits((uint32_t)v << 4)/divisor;
}
static int32_t field(const SniperMoveShip *s,const int32_t *object,unsigned offset) {
    if (object) return object[offset/4];
    switch (offset) {
    case 0x44:return s->player_id;
    case 0x60:return s->death_ticks;
    case 0x64:return s->respawn_ticks;
    default:return s->attached_id;
    }
}
static int tile(SniperMoveWorld *w, SniperMoveShip *s,const int32_t *object,
    int32_t x,int32_t y,int effects) {
    int t = x < 0 || y < 0 || x >= 1024 || y >= 1024 ? 20 : w->map[y*1024+x];
    if (t == 152) return 0;
    if (t != 255) return t;
    if (!effects || field(s,object,0x60) > 0 || field(s,object,0x64) > 0 || *s->ship_x == -1) return 0;
    for (int32_t i=0;i<w->pickup_count;++i) {
        SniperPickup *p=&w->pickups[i];
        if (p->x != x || p->y != y) continue;
        int32_t player=field(s,object,0x44);
        int local = player == w->local_id && field(s,object,0x6c) == -1;
        if (w->pickup) w->pickup(w->user,x,y,player,p->value,local);
        break;
    }
    return 0;
}
static void move_fields(int32_t *x,int32_t *y,int32_t *vx,int32_t *vy,
    SniperMoveShip *s,const int32_t *object,SniperMoveWorld *w,int32_t ticks,int effects) {
    int near = s->player_id == w->local_id || s->player_id == w->watched_id ||
        (abs32(sub(sub((*x)/1000,w->camera_x),320)) < 400 &&
         abs32(sub(sub((*y)/1000,w->camera_y),240)) < 300);
    int impact=0;
    for (int32_t step=0;step<ticks;++step) {
        (*x)=add((*x),(*vx));
        if (near || (step & 3)==0) {
            int32_t lo=sub((*y),14000)/16000, hi=add((*y),14000)/16000;
            int left=(*vx)<0;
            int32_t edge=left ? sub((*x),14000)/16000 : add((*x),14000)/16000;
            if (left && (*x)<14000) edge=-1;
            int solid=0;
            for (int32_t y=lo;y<=hi;++y) if (tile(w,s,object,edge,y,effects)>0) solid=1;
            if (solid) {
                if (abs32((*vx))>1000) impact=1;
                (*x)=add(bits((uint32_t)edge*16000u),left ? 30000 : -14001);
                (*vx)=neg(damp((*vx),w->bounce_divisor));
                (*vy)=damp((*vy),w->bounce_divisor);
            }
        }
        (*y)=add((*y),(*vy));
        if (near || (step & 3)==0) {
            int32_t lo=sub((*x),14000)/16000, hi=add((*x),14000)/16000;
            int up=(*vy)<0;
            int32_t edge=up ? sub((*y),14000)/16000 : add((*y),14000)/16000;
            if (up && (*y)<14000) edge=-1;
            int solid=0;
            for (int32_t x=lo;x<=hi;++x) if (tile(w,s,object,x,edge,effects)>0) solid=1;
            if (solid) {
                if (abs32((*vy))>1000) impact=1;
                (*y)=add(bits((uint32_t)edge*16000u),up ? 30000 : -14001);
                (*vx)=damp((*vx),w->bounce_divisor);
                (*vy)=neg(damp((*vy),w->bounce_divisor));
            }
        }
    }
    if (effects && impact && w->impact_sound) w->impact_sound(w->user,(*x)/1000,(*y)/1000);
}

void sniper_move(SniperMotion *m,SniperMoveShip *s,SniperMoveWorld *w,int32_t ticks,int effects) {
    sniper_move_fields(&m->x,&m->y,&m->vx,&m->vy,s,w,ticks,effects);
}

void sniper_move_fields(int32_t *x,int32_t *y,int32_t *vx,int32_t *vy,
    SniperMoveShip *s,SniperMoveWorld *w,int32_t ticks,int effects) {
    move_fields(x,y,vx,vy,s,0,w,ticks,effects);
}
void sniper_move_object(int32_t *object,SniperMoveWorld *w,int32_t ticks,int effects) {
    SniperMoveShip ship={object[0x44/4],object[0x60/4],object[0x64/4],
        object[0x6c/4],&object[0x48/4]};
    move_fields(&object[0x48/4],&object[0x4c/4],&object[0x50/4],&object[0x54/4],
        &ship,object,w,ticks,effects);
}
