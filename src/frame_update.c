#include "frame_update.h"
#include <string.h>
static int32_t sub(int32_t a,uint32_t b) {
    uint32_t u=(uint32_t)a-b;int32_t s;memcpy(&s,&u,4);return s;
}
void sniper_frame_update(SniperFrameUpdate *f,uint32_t milliseconds) {
    sniper_frame_clock_step(&f->clock,milliseconds);
    if(f->auto_pending && f->local && f->local->word[0x5c/4]>500000)
        f->post_command(f->user);
    f->backend(f->user);
    f->input(f->user);
    if(!f->special) {
        if(f->safe_timer>0) {
            f->safe_timer=sub(f->safe_timer,f->clock.elapsed);
            if(f->safe_timer<=0)f->post_command(f->user);
        }
        int32_t previous=f->frequency;
        f->frequency%=f->frequency_count;
        if(previous!=f->frequency) {
            f->team(f->user,f->frequency);
            if(f->local->word[0x6c/4]!=-1)f->detach(f->user,-1);
        }
        f->simulate(f->user);
    }
    f->camera=sniper_camera(f->local,f->watched,f->width,f->height,
        f->special,f->tile_x,f->tile_y,f->shake,&f->random_state);
}
