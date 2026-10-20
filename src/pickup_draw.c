#include "pickup_draw.h"
#include <string.h>
const SniperSprite sniper_pickup_sprite={142,0,0,16,16,1,10,120,10};
static int32_t screen(int32_t tile,int32_t camera) {
    uint32_t u=((uint32_t)tile<<4)-(uint32_t)camera;int32_t s;
    memcpy(&s,&u,4);return s;
}
void sniper_pickup_draw(const SniperPickupObject *p,int32_t camera_x,int32_t camera_y,
    SniperPickupDraw draw,void *user) {
    draw(user,screen(p->x,camera_x),screen(p->y,camera_y),(p->age/12)%10,0,0);
}
