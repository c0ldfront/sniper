#ifndef SNIPER87_MACRO_DIALOG_H
#define SNIPER87_MACRO_DIALOG_H
#include <windows.h>
/* Template152. Updates text only on OK; audio recording/import operates on
 * record0.wav/record1.wav immediately, including when Cancel follows. */
INT_PTR sniper87_macro_dialog(HWND owner,HINSTANCE instance,char macros[8][82]);
#endif
