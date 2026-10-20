#ifndef SNIPER87_CONNECTION_DIALOG_H
#define SNIPER87_CONNECTION_DIALOG_H
#include <windows.h>
#include <stddef.h>
typedef struct {char name[80],host[64],port[8];} Sniper87ArenaEntry;
typedef struct {
    char player_name[32],password[32],arena[128],host[136],port[8];
} Sniper87ConnectionSettings;
/* Original arena.lst records; safe maximum32 from original zeroed allocation. */
size_t sniper87_connection_read_arenas(const char *,Sniper87ArenaEntry entries[32]);
/* Returns0 for valid selection,1 for invalid manual host. Mutates host/port. */
int sniper87_connection_resolve(Sniper87ConnectionSettings *,const Sniper87ArenaEntry *,size_t);
/* Resource136. Returns1 OK,2 Quick Start,0 Cancel/close,-1 creation failure.
 * Original edits mutate settings on attempted acceptance,even if validation fails.
 * No network connection is made. Caller owns profile persistence. */
INT_PTR sniper87_connection_dialog(HWND,HINSTANCE,Sniper87ConnectionSettings *);
#endif
