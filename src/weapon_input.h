#ifndef SNIPER_WEAPON_INPUT_H
#define SNIPER_WEAPON_INPUT_H
#include <stdint.h>
typedef struct {
    int32_t joystick_enabled,joystick_buttons,joystick_pov;
    int32_t mouse_pressed,mouse_x,mouse_y,vertical_extent;
    int32_t (*key_state)(void *,int32_t virtual_key);
    void *user;
} SniperWeaponInput;
typedef struct {int32_t active,action;} SniperWeaponSelection;
/*406332..406431. Win32 high bit0x8000 denotes pressed. Cooldown wraps, no clamp.
 * Poll order Control,[Shift iff Control],Tab; later sources override earlier. */
SniperWeaponSelection sniper_weapon_input(const SniperWeaponInput *,int32_t ticks,int32_t *cooldown);
#endif
