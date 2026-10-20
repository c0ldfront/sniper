#ifndef SNIPER87_SHIP_DIALOG_H
#define SNIPER87_SHIP_DIALOG_H

#include <windows.h>
#include <stdint.h>

typedef struct Sniper87ShipDialogState {
    /* Original globals 42e148 and 42227c. Ship 0..3, spectator 4.
     * Configuration defaults are ship 0, history 4. No dialog clamping. */
    uint32_t ship_index;
    int32_t history_size;
} Sniper87ShipDialogState;

/* Resource 110 must be linked into instance. Returns 1 for OK, 0 for cancel
 * or titlebar close, -1 for creation failure. State is changed only by OK.
 * OK also writes [Dialog] PlayerShipPicked=1 through the original relative
 * profile filename "sniper.ini". Saving the state itself belongs to caller's
 * existing configuration lifecycle. This routine does not start a game. */
INT_PTR sniper87_ship_dialog(HWND owner, HINSTANCE instance,
                             Sniper87ShipDialogState *state);

#endif
