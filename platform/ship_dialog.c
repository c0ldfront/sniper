/* Recovered from Sniper .87: callback 40d960; characteristics 40dad0.
 */
#define WIN32_LEAN_AND_MEAN
#include "ship_dialog.h"

static void characteristics(HWND dialog, uint32_t ship)
{
    static const char *const rows[5][3] = {
        {"Very maneuverable", "", ""},
        {"High velocity", "Quick charging", ""},
        {"Stealth", "(radar invisible)", "Extra energy"},
        {"Extra energy", "Bombs", ""},
        {"Views other players", "(not participating)", ""}
    };
    int id;
    /* Original always clears rows 2..5, but leaves row 1 unchanged if the
     * stored ship is outside 0..4. Preserve that detail (including negatives). */
    for (id = 1014; id <= 1017; ++id)
        SetWindowTextA(GetDlgItem(dialog, id), "");
    if (ship <= 4) {
        SetWindowTextA(GetDlgItem(dialog, 1013), rows[ship][0]);
        if (ship != 0)
            SetWindowTextA(GetDlgItem(dialog, 1014), rows[ship][1]);
        if (ship == 2)
            SetWindowTextA(GetDlgItem(dialog, 1015), rows[ship][2]);
    }
}

static INT_PTR CALLBACK ship_proc(HWND dialog, UINT message,
                                  WPARAM wparam, LPARAM lparam)
{
    Sniper87ShipDialogState *state =
        (Sniper87ShipDialogState *)GetWindowLongPtrA(dialog, DWLP_USER);
    switch (message) {
    case WM_INITDIALOG:
        state = (Sniper87ShipDialogState *)lparam;
        SetWindowLongPtrA(dialog, DWLP_USER, (LONG_PTR)state);
        CheckRadioButton(dialog, 1001, 1005, state->ship_index + 1001u);
        SetDlgItemInt(dialog, 1035, (UINT)state->history_size, TRUE);
        characteristics(dialog, state->ship_index);
        return TRUE;
    case WM_CLOSE:
        EndDialog(dialog, (INT_PTR)wparam);
        return TRUE;
    case WM_COMMAND: {
        unsigned id = LOWORD(wparam);
        if (id == IDOK) {
            unsigned ship;
            WritePrivateProfileStringA("Dialog", "PlayerShipPicked", "1",
                                       "sniper.ini");
            state->history_size = (int32_t)GetDlgItemInt(dialog, 1035, NULL, TRUE);
            for (ship = 0; ship <= 4; ++ship) {
                if (IsDlgButtonChecked(dialog, (int)ship + 1001) != 0)
                    state->ship_index = ship;
            }
            PostMessageA(dialog, WM_CLOSE, 1, 0);
        } else if (id == IDCANCEL) {
            PostMessageA(dialog, WM_CLOSE, 0, 0);
        } else if (id >= 1001 && id <= 1005) {
            characteristics(dialog, id - 1001);
        }
        /* Original consumes all WM_COMMAND, regardless of notification code. */
        return TRUE;
    }
    default:
        return FALSE;
    }
}

INT_PTR sniper87_ship_dialog(HWND owner, HINSTANCE instance,
                             Sniper87ShipDialogState *state)
{
    if (!state) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    return DialogBoxParamA(instance, MAKEINTRESOURCEA(110), owner,
                           ship_proc, (LPARAM)state);
}
