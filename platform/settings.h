#ifndef SNIPER87_SETTINGS_H
#define SNIPER87_SETTINGS_H
#include <windows.h>
#include "../src/options.h"
void sniper_options_load(SniperOptions *options);
void sniper_options_save(const SniperOptions *options);
void sniper_options_menu(HMENU menu,const SniperOptions *options);
#endif
