#ifndef SNIPER_ORDINARY_CONTROL_H
#define SNIPER_ORDINARY_CONTROL_H
#include "local_control.h"
#include "ship_flight.h"
#include "weapon_control.h"
typedef struct {
    SniperLocalControl *lifecycle;
    SniperWeaponActionWorld *weapons;
    SniperPositionDispatch *position;
    SniperWeaponInput input; /* mouse geometry and key callback; joystick replaced after poll */
    int32_t ticks,engine_sound,speed_exempt,exhaust_ticks;
    /* Explicit incoming stack scratch. fields[3] must equal input.mouse_pressed
     * on entry: both represent ESP+14 before weapon selection. */
    int32_t fields[6];
    SniperFlightEffects effects;
    int dispatched;
    void (*audio_effects)(void *,const SniperFlightEffects *);
    void *audio_user;
} SniperOrdinaryControl;
/* Composes405f75..406aa6. lifecycle.local is the authoritative shared ship.
 * Damage and joystick callbacks retain lifecycle.user. Key sampling uses
 * input.user. Weapon/backend callbacks retain their own contexts.
 * Sound calls remain ordered effects; audio delivery is the host's job.
 * Dead gate leaves stage state/effects untouched and returns0; otherwise1.
 * This excludes earlier command/menu/chat/spectator handling. */
int sniper_ordinary_control(SniperOrdinaryControl *);
#endif
