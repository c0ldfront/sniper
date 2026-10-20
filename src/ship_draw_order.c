#include "ship_draw_order.h"
void sniper_ship_draw_order(SniperShipDrawOrder *w){
    uint32_t frame=w->frame++;
    if(!(frame&15u) && w->count>0){
        int32_t leader=0;
        for(int32_t i=0;i<w->count;++i){
            if(w->ships[i]->word[0x80/4]>w->ships[leader]->word[0x80/4])leader=i;
            w->ships[i]->word[0x84/4]=0;
        }
        w->ships[leader]->word[0x84/4]=1;
    }
    if(w->count==1)w->ships[0]->word[0x84/4]=0;
    for(int pass=0;pass<2;++pass){
        for(int32_t i=0;i<w->count;++i){
            SniperShipObject *s=w->ships[i];
            if(s->word[0x44/4]==w->local_id || (s->word[0x6c/4]!=-1)!=pass)continue;
            if(s->word[0x68/4]!=w->local->word[0x68/4] && !w->local->word[0x94/4] && s->word[0x8c/4])continue;
            w->draw(w->user,s,w->camera_x,w->camera_y);
        }
        if((w->local->word[0x6c/4]!=-1)==pass)w->draw(w->user,w->local,w->camera_x,w->camera_y);
    }
}
