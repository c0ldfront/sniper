/* Profile semantics recovered from 40d070/40d530; menu from 40ae10. */
#include "settings.h"
#include <stdio.h>
static const char *keys[SNIPER_OPTION_COUNT]={"Joystick","LowDetail","ShowStars",
    "PlayerReceiveWaveMessages","Sound","ThrustSound","SmallRadar","ShowEnergyBar","TickerOn"};
static const char *section(unsigned i) { return i==SNIPER_RECEIVE_WAVE?"Dialog":"Misc"; }
void sniper_options_load(SniperOptions *s) {
    sniper_options_default(s);
    for (unsigned i=0;i<SNIPER_OPTION_COUNT;++i)
        s->value[i]=(int32_t)GetPrivateProfileIntA(section(i),keys[i],s->value[i],"sniper.ini");
}
void sniper_options_save(const SniperOptions *s) {
    for (unsigned i=0;i<SNIPER_OPTION_COUNT;++i) {
        char value[32];snprintf(value,sizeof value,"%ld",(long)s->value[i]);
        WritePrivateProfileStringA(section(i),keys[i],value,"sniper.ini");
    }
}
void sniper_options_menu(HMENU menu,const SniperOptions *s) {
    for (unsigned i=0;i<SNIPER_OPTION_COUNT;++i)
        CheckMenuItem(menu,sniper_option_commands[i],MF_BYCOMMAND|(s->value[i]?MF_CHECKED:MF_UNCHECKED));
    EnableMenuItem(menu,40027,MF_BYCOMMAND|(s->value[SNIPER_SOUND]?MF_ENABLED:MF_GRAYED));
    EnableMenuItem(menu,40025,MF_BYCOMMAND|(s->value[SNIPER_SOUND]?MF_ENABLED:MF_GRAYED));
}
