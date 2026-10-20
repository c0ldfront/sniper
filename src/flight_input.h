#ifndef SNIPER_FLIGHT_INPUT_H
#define SNIPER_FLIGHT_INPUT_H
#include <stdint.h>
enum { SNIPER_UP=1, SNIPER_DOWN=2, SNIPER_LEFT=4, SNIPER_RIGHT=8, SNIPER_SHIFT=16 };
typedef struct {
    int32_t heading, rotation_rate, lock_ticks, attached_id;
} SniperFlightControl;
typedef struct { uint32_t keys, joystick_enabled, joystick_x, joystick_y; } SniperFlightInput;
/* Updates heading (40000 units/revolution); returns thrust -1,0,+1.
 * Original input block 0x405ffa..0x40610d. Does not apply acceleration. */
int32_t sniper_flight_input(SniperFlightControl *, const SniperFlightInput *, int32_t ticks);
/* Poll callback returns the Win32 key-state bits. It may return different
 * values on successive calls; it must not mutate control/joystick state. */
typedef int32_t (*SniperFlightKeyState)(void *,int32_t virtual_key);
int32_t sniper_flight_input_polled(SniperFlightControl *,const SniperFlightInput *,
                                  int32_t,SniperFlightKeyState,void *);
#endif
