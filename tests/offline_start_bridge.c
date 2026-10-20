#include "offline_start.h"
static int calls;
static void cursor(void *user,int32_t x,int32_t y){(void)user;if(x==0&&y==0)++calls;}
/* values:ms,seed,dirty,height,history,force4,selected,watched,frequency,listdirty,cursor tick,calls */
void test_offline_start(SniperShipObject ships[12],int32_t order[12],uint8_t *map,int32_t values[12]) {
    SniperShipWorld w={0};SniperShipObject *ordered[12];sniper_local_config(&w.config);
    for(int i=0;i<12;++i){w.players[i]=ships+i;ordered[i]=ships+order[i];}
    w.player_count=12;w.movement.map=map;w.milliseconds=(uint32_t)values[0];
    w.random_state=(uint32_t)values[1];w.position_dirty=values[2];
    SniperOfflineStart s={0};s.height=values[3];s.history_size=values[4];s.force_four=values[5];
    s.selected=values[6];s.cursor=cursor;calls=0;
    sniper_offline_start(&w,ordered,&s,(uint32_t)values[0]);
    for(int i=0;i<12;++i)order[i]=(int32_t)(ordered[i]-ships);
    values[1]=(int32_t)w.random_state;values[2]=w.position_dirty;values[4]=s.history_size;
    values[6]=s.selected;values[7]=s.watched;values[8]=s.frequency;values[9]=s.list_dirty;
    values[10]=(int32_t)s.cursor_tick;values[11]=calls;
}
