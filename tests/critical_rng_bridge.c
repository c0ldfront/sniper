/* Exercise the real private session wiring, not a duplicate sound adapter. */
#include "../src/offline_session.c"
static SniperOfflineSession session;
static SniperShipObject player;
static int32_t *capture_out;
static void capture_request(void *user,int32_t resource,const SniperAudioRequest *r){
    (void)user;capture_out[0]++;capture_out[1]=resource;
    capture_out[2]=r->volume;capture_out[3]=r->pan;
    capture_out[4]=r->loop;capture_out[5]=r->pitch;
}
__attribute__((visibility("default")))
void critical_rng_reset(uint32_t seed){
    memset(&session,0,sizeof(session));memset(&player,0,sizeof(player));
    session.start.local_id=0;session.ships.random_state=seed;
}
__attribute__((visibility("default")))
void critical_rng_step(const int32_t args[10],int32_t out[12]){
    memset(out,0,12*sizeof(*out));capture_out=out;
    session.ships.players[0]=args[6]?&player:NULL;
    player.word[0x48/4]=args[2];player.word[0x4c/4]=args[3];
    session.frame.width=args[4];
    session.ships.config.word[(0x4282a5-0x4281c9)/4]=args[5];
    session.host.audio=args[7]?capture_request:NULL;
    if(args[8]==1)prize_sound(&session,148);
    else if(args[8]==2)impact(&session,args[0],args[1]);
    else sound(&session,162,args[0],args[1]);
    out[6]=(int32_t)session.ships.random_state;
    for(int i=0;i<args[9];++i)out[7+i]=(int32_t)sniper_random(&session.ships.random_state);
    out[11]=(int32_t)session.ships.random_state;
}
