#include "options.h"
const unsigned sniper_option_commands[SNIPER_OPTION_COUNT]={40021,40023,40024,
    40025,40026,40027,40028,40029,40032};
void sniper_options_default(SniperOptions *s) {
    *s=(SniperOptions){{0,0,1,0,1,1,0,1,1}};
}
int sniper_options_command(SniperOptions *s,unsigned command) {
    for (unsigned i=0;i<SNIPER_OPTION_COUNT;++i) {
        if (command!=sniper_option_commands[i]) continue;
        s->value[i]=!s->value[i];
        if (i==SNIPER_LOW_DETAIL) {
            s->value[SNIPER_ENERGY_BAR]=!s->value[i];
            s->value[SNIPER_STARS]=!s->value[i];
            s->value[SNIPER_SMALL_MAP]=s->value[i];
            s->value[SNIPER_ENGINE_SOUND]=!s->value[i];
        }
        return 1;
    }
    return 0;
}
