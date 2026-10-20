#include "radar_markers.h"
#include <string.h>
static int32_t bits(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t add(int32_t a,int32_t b){return bits((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return bits((uint32_t)a-(uint32_t)b);}
static int32_t coordinate(int32_t tile,int32_t extent,int32_t start){return sub(bits((uint32_t)tile*(uint32_t)extent)/1024,start);}
static int32_t tile(int32_t pos){int32_t t=pos/16000;return t<0?-t:t;}
static void draw(const SniperRadarMarkers *s,int32_t tx,int32_t ty,uint32_t color) {
    int32_t x=coordinate(tx,s->map_width,s->view.source_x),y=coordinate(ty,s->map_height,s->view.source_y);
    if(x<0||y<0||x>=s->view.size||y>=s->view.size)return;
    int32_t side=s->screen_width==320?1:2;
    x=add(x,s->view.destination_x);y=add(y,s->view.destination_y);
    s->rect(s->user,x,y,add(x,side),add(y,side),color);
}
void sniper_radar_markers(const SniperRadarMarkers *s) {
    if(s->special){draw(s,s->tile_x,s->tile_y,s->projectile_color);return;}
    for(int32_t i=0;i<s->ship_count;++i) {
        SniperShipObject *p=s->ships[i];int32_t id=p->word[0x44/4];
        if(p->word[0xa4/4]==2||p->word[0x48/4]==-1||p->word[0x40/4]==4)continue;
        if((id==s->local_id||id==s->watched_id||id==s->local->word[0x6c/4])&&(s->tick/16)%2)continue;
        int friendly=s->reference&&p->word[0x68/4]==s->reference->word[0x68/4];
        if(friendly){if(p->word[0x6c/4]!=-1)continue;}
        else if(id!=s->local_id&&!s->local->word[0x94/4]&&p->word[0x88/4])continue;
        uint32_t color=s->friendly;
        if(!friendly) {
            int32_t bounty=p->word[0xa0/4],delta=sub(bounty,s->local->word[0xa0/4]);
            if(delta>=s->bounty_upper)delta=s->bounty_upper;
            int32_t lower=bits(0u-(uint32_t)s->bounty_lower);
            if(delta<=lower)delta=lower;
            color=add(add(delta,s->bounty_bonus),bounty)>0?s->enemy_positive:s->enemy_nonpositive;
        }
        draw(s,tile(p->word[0x48/4]),tile(p->word[0x4c/4]),color);
    }
    for(int32_t i=0;i<s->pickup_count;++i)draw(s,s->pickups[i]->x,s->pickups[i]->y,s->pickup_color);
    if(!((s->tick/8)%2))return;
    for(int32_t i=0;i<s->projectile_count;++i) {
        SniperProjectile *p=s->projectiles[i];
        if(p->word[0x18/4]<=0||p->word[0x20/4]!=s->local->word[0x68/4]||
            (p->word[0x34/4]!=3&&p->word[0x34/4]!=4))continue;
        draw(s,tile(p->word[1]),tile(p->word[2]),s->projectile_color);
    }
}
