#include "offline_team.h"
/* values:selected,dirty,reset,milliseconds,random_state,position_dirty,frequency */
void test_offline_team(SniperShipObject ships[65],int32_t order[65],int32_t count,int32_t values[7]) {
    SniperShipWorld w={0};SniperShipObject *ordered[65];
    sniper_local_config(&w.config);w.config.word[(0x4282ad-0x4281c9)/4]=values[2];
    w.player_count=count;w.milliseconds=(uint32_t)values[3];w.random_state=(uint32_t)values[4];
    w.position_dirty=values[5];w.movement.local_id=0;w.offline_backend=1;
    for(int i=0;i<65;++i)w.players[i]=ships+i;
    for(int i=0;i<count;++i)ordered[i]=ships+order[i];
    sniper_offline_team(&w,ordered,values,values+1,values[6]);
    for(int i=0;i<count;++i)order[i]=(int32_t)(ordered[i]-ships);
    values[4]=(int32_t)w.random_state;values[5]=w.position_dirty;
}
