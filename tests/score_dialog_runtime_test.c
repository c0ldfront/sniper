#include "score_dialog.h"
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
static int mode,failed;
static void check(int condition,const char *why){if(!condition){printf("FAIL %s mode%d\n",why,mode);failed=1;}}
static VOID CALLBACK drive(HWND ignored,UINT message,UINT_PTR timer,DWORD time){
 (void)ignored;(void)message;(void)time;
 HWND h=FindWindowA(NULL,mode?"Squad Scores":"Player Scores");if(!h)return;
 KillTimer(NULL,timer);HWND list=GetDlgItem(h,1010);char text[100];
 check(ListView_GetItemCount(list)==3,"row count");
 check(Header_GetItemCount(ListView_GetHeader(list))==(mode?10:7),"column count");
 LVFINDINFOA find;memset(&find,0,sizeof find);find.flags=LVFI_STRING;find.psz="Beta";int beta=(int)SendMessageA(list,LVM_FINDITEMA,(WPARAM)-1,(LPARAM)&find);check(beta>=0,"Beta row");
 ListView_GetItemText(list,beta,mode?9:6,text,sizeof text);check(!strcmp(text,"5.0"),"points per win");
 if(mode){ListView_GetItemText(list,beta,6,text,sizeof text);check(!strcmp(text,"3"),"integer wins/member");}
 NM_LISTVIEW click;memset(&click,0,sizeof click);click.hdr.hwndFrom=list;click.hdr.code=LVN_COLUMNCLICK;click.iSubItem=2;
 SendMessageA(h,WM_NOTIFY,1010,(LPARAM)&click);
 ListView_GetItemText(list,0,0,text,sizeof text);check(!strcmp(text,"Zulu"),"descending win sort");
 click.iSubItem=0;SendMessageA(h,WM_NOTIFY,1010,(LPARAM)&click);
 ListView_GetItemText(list,0,0,text,sizeof text);check(!strcmp(text,"Alpha"),"case insensitive name sort");
 if(!mode){click.iSubItem=1;SendMessageA(h,WM_NOTIFY,1010,(LPARAM)&click);ListView_GetItemText(list,2,1,text,sizeof text);check(!*text,"empty squad last");}
 MoveWindow(h,40,50,600,300,TRUE);RECT r;GetClientRect(h,&r);uint16_t widths[10];sniper87_score_widths(mode,r.right,GetSystemMetrics(SM_CXVSCROLL),widths);
 for(int i=0;i<(mode?10:7);++i)check(ListView_GetColumnWidth(list,i)==widths[i],"resize width");
 SendMessageA(h,WM_COMMAND,mode?IDCANCEL:IDOK,0);
}
int main(void){
 SniperPlayerScore p[3]={{"Beta","",10,2,50},{"Zulu","Alpha",100,8,21},{"Alpha","Beta",0,5,-10}};
 SniperSquadScore s[3]={{"Beta",10,2,50,3},{"Zulu",100,8,21,4},{"Alpha",0,5,-10,1}};
 for(mode=0;mode<2;++mode){SetTimer(NULL,1,50,drive);INT_PTR result=mode?sniper87_squad_scores_dialog(NULL,GetModuleHandle(NULL),s,3):sniper87_player_scores_dialog(NULL,GetModuleHandle(NULL),p,3);check(result==0,"dialog return");const char *section=mode?"SquadScores":"Scores";check(GetPrivateProfileIntA(section,"Width",0,"sniper.ini")==600,"saved width");check(GetPrivateProfileIntA(section,"Height",0,"sniper.ini")==300,"saved height");}
 printf("%s real templates144/175: rows,columns,averages,name/win/squad sorts,resize,INI geometry,OK/Cancel\n",failed?"FAIL":"PASS");return failed;
}
