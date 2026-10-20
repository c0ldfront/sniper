#include "offline_frame.h"
#include <string.h>
typedef struct {SniperOfflineFrame *binding;SniperFrameUpdate original;} Context;
static void sync_camera(SniperOfflineFrame *b) {
    b->frame->local=b->combat->ships->players[b->combat->ships->movement.local_id];
    b->frame->random_state=b->combat->ships->random_state;
    b->frame->shake=b->combat->shake;
}
static void post(void *user) {
    Context *c=user;c->original.post_command(c->original.user);
}
static void backend(void *user) {
    Context *c=user;
    if(c->binding->backend_override)c->binding->backend_override(c->binding->backend_user);
    else sniper_offline_pickups_generate(c->binding->pickups);
    sync_camera(c->binding);
}
static void input(void *user) {
    SniperOfflineFrame *b=((Context *)user)->binding;
    b->control->ticks=(int32_t)b->frame->clock.elapsed;
    memcpy(&b->control->position->ticks,&b->frame->clock.tick,4);
    b->control->position->interval=b->combat->ships->config.word[(0x428205-0x4281c9)/4];
    b->control->speed_exempt=b->combat->repel_timer>0;
    if(b->input)b->input(b->input_user,b);
    else sniper_offline_control(b->combat,b->control);
    b->frame->safe_timer=b->combat->safe_timer;
    sync_camera(b);
}
static void team(void *user,int32_t frequency) {
    SniperOfflineFrame *b=((Context *)user)->binding;
    if(b->team_override)b->team_override(b->backend_user,frequency);
    else sniper_offline_team(b->combat->ships,b->combat->ordered_ships,
        &b->selected,&b->list_dirty,frequency);
    sync_camera(b);
}
static void detach(void *user,int32_t target) {
    SniperOfflineFrame *b=((Context *)user)->binding;
    if(b->detach_override)b->detach_override(b->backend_user,target);
    else sniper_ship_world_offline_attach(b->combat->ships,target);
}
static void simulate(void *user) {
    SniperOfflineFrame *b=((Context *)user)->binding;
    b->combat->safe_timer=b->frame->safe_timer;
    int32_t ticks=(int32_t)b->frame->clock.elapsed;
    if(b->simulate_override)b->simulate_override(b->backend_user,&ticks);
    else sniper_offline_simulation(b->combat,b->pickups,&ticks);
    b->frame->safe_timer=b->combat->safe_timer;
    sync_camera(b);
}
int sniper_offline_frame(SniperOfflineFrame *b,uint32_t milliseconds) {
    SniperFrameUpdate *f=b->frame;
    if(!b->input && (f->special || b->combat->ships->players[b->combat->ships->movement.local_id]->word[0x40/4]==4))return 0;
    Context context={b,*f};
    b->combat->ships->milliseconds=milliseconds;
    f->safe_timer=b->combat->safe_timer;
    f->frequency_count=b->combat->ships->config.word[(0x4282a9-0x4281c9)/4];
    sync_camera(b);
    f->post_command=post;f->backend=backend;f->input=input;
    f->team=team;f->detach=detach;f->simulate=simulate;f->user=&context;
    sniper_frame_update(f,milliseconds);
    b->combat->ships->random_state=f->random_state;
    b->combat->safe_timer=f->safe_timer;
    f->post_command=context.original.post_command;f->backend=context.original.backend;
    f->input=context.original.input;f->team=context.original.team;f->detach=context.original.detach;
    f->simulate=context.original.simulate;f->user=context.original.user;
    return 1;
}
