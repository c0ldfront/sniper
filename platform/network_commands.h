#ifndef SNIPER87_NETWORK_COMMANDS_H
#define SNIPER87_NETWORK_COMMANDS_H
#include <stddef.h>
#include "score_dialog.h"
/* Pure recovered data stages; returned arrays are free-compatible. */
int sniper87_network_score_parse(const uint8_t *,size_t,SniperPlayerScore **,int *);
int sniper87_network_squad_scores(SniperPlayerScore *,int,SniperSquadScore **,int *);
int sniper87_network_squad_packet(uint8_t packet[65],const char *name,const char *password);
#ifdef _WIN32
#include "../src/network_client.h"
/*1recognized command,0unrecognized. Requires idle connected client; synchronous
 * score/squad waits retain native Escape/deadline behavior and forward game
 * events. No state survives this call. */
int sniper87_network_command(HWND,HINSTANCE,SniperNetworkClient *,unsigned command);
#endif
#endif
