#ifndef SNIPER87_SCORE_DIALOG_H
#define SNIPER87_SCORE_DIALOG_H
#include <stdint.h>
/* Wire/file layouts consumed by native 40e1f0 and 40ead0. */
typedef struct {char name[32],squad[32];int16_t wins,losses;int32_t points;} SniperPlayerScore;
typedef struct {char name[32];int16_t wins,losses;int32_t points,members;} SniperSquadScore;
int sniper87_player_score_compare(const SniperPlayerScore *,const SniperPlayerScore *,int column);
int sniper87_squad_score_compare(const SniperSquadScore *,const SniperSquadScore *,int column);
/* Sorts players in place using recovered CRT tie ordering; excludes empty and
 * singleton squads. Output capacity count/2 suffices. Returns count or -1. */
int sniper87_build_squad_scores(SniperPlayerScore *,int count,SniperSquadScore *,int capacity);
/* Returns 0 on invalid native division inputs; otherwise fills all cells. */
int sniper87_player_score_cells(const SniperPlayerScore *,char cells[10][64]);
int sniper87_squad_score_cells(const SniperSquadScore *,char cells[10][64]);
void sniper87_score_widths(int squad,int client_width,int scrollbar_width,uint16_t widths[10]);
#ifdef _WIN32
#include <windows.h>
/* Arrays borrowed for duration of modal call. Templates144 and175, control1010. */
INT_PTR sniper87_player_scores_dialog(HWND,HINSTANCE,const SniperPlayerScore *,int count);
INT_PTR sniper87_squad_scores_dialog(HWND,HINSTANCE,const SniperSquadScore *,int count);
#endif
#endif
