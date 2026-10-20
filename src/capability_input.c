#include "capability_input.h"
int sniper_capability_input(SniperShipObject *s,int32_t key,int32_t shift,int32_t *dirty) {
    unsigned available,active;
    switch(key) {
    case 0x23:available=(shift&0x8000)?0xf4:0xf8;active=(shift&0x8000)?0x90:0x94;break;
    case 0x24:available=(shift&0x8000)?0xf0:0xec;active=(shift&0x8000)?0x8c:0x88;break;
    case 0x2e:available=0xfc;active=0x98;break;
    default:return 0;
    }
    if(s->word[available/4]) {
        s->word[active/4]=!s->word[active/4];
        if(active==0x8c && !s->word[active/4])*dirty=1;
    }
    return 1;
}
