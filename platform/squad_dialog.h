#ifndef SNIPER87_SQUAD_DIALOG_H
#define SNIPER87_SQUAD_DIALOG_H
#include <windows.h>
typedef struct {char name[32],password[32];} Sniper87SquadCredentials;
/* Original174 always opens with empty edits; commits only on OK. */
INT_PTR sniper87_squad_dialog(HWND,HINSTANCE,Sniper87SquadCredentials *);
#endif
