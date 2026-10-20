#include "offline_generation.h"
#include "ship_spawn.h"
void sniper_offline_generate(uint32_t *previous,uint32_t *random_state,
    const uint8_t *map,uint32_t (*clock_ms)(void *),
    void (*create)(void *,int32_t,int32_t,int32_t,int32_t),void *user) {
    uint32_t now=clock_ms(user)/10u;
    if(now-*previous<=500u)return;
    *previous=clock_ms(user)/10u;
    int32_t x,y;
    do {
        x=(int32_t)(sniper_random(random_state)%264u)+380;
        y=(int32_t)(sniper_random(random_state)%264u)+380;
    }while(map[y*1024+x]!=0);
    int32_t lifetime=(int32_t)(sniper_random(random_state)%7000u)+6000;
    int32_t code=(int32_t)(sniper_random(random_state)%21u)+1;
    create(user,x,y,code,lifetime);
}
