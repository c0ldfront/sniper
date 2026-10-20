#ifndef SNIPER87_OPTIONS_H
#define SNIPER87_OPTIONS_H
#include <stdint.h>
enum { SNIPER_JOYSTICK, SNIPER_LOW_DETAIL, SNIPER_STARS, SNIPER_RECEIVE_WAVE,
       SNIPER_SOUND, SNIPER_ENGINE_SOUND, SNIPER_SMALL_MAP, SNIPER_ENERGY_BAR,
       SNIPER_TICKER, SNIPER_OPTION_COUNT };
typedef struct { int32_t value[SNIPER_OPTION_COUNT]; } SniperOptions;
extern const unsigned sniper_option_commands[SNIPER_OPTION_COUNT];
void sniper_options_default(SniperOptions *options);
/* Returns 1 when a known command is handled, 0 otherwise. */
int sniper_options_command(SniperOptions *options, unsigned command);
#endif
