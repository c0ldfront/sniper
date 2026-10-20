#include "ship_update_order.h"
void sniper_ship_update_order(SniperShipObject **ordered,const int32_t *count,
    const int32_t *ticks,SniperOrderedUpdate update,void *user) {
    for(int pass=0;pass<2;++pass) {
        for(int32_t i=0;i<*count;++i) {
            SniperShipObject *ship=ordered[i];
            int attached=ship->word[0x6c/4]!=-1;
            if(attached==pass)update(user,ship,*ticks);
        }
    }
}
