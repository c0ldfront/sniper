/* Read-only visual parity fixture: menu messages to an independently running
 * client. No memory edits,code hooks or executable patches. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
int main(int argc,char **argv){
 (void)argv;
 HWND main=FindWindowA("SniperClass",NULL);if(!main){puts("no main window");return 1;}
 HWND connect=FindWindowA(NULL,"Connection Information");if(connect){PostMessageA(connect,WM_COMMAND,IDCANCEL,0);Sleep(1000);}
 PostMessageA(main,WM_COMMAND,40024,0); /* default stars1 ->0 */
 Sleep(200);
 PostMessageA(main,WM_COMMAND,argc>1?40008:40009,0);
 if(argc>1){
  HWND game=NULL;for(int i=0;i<50&&!game;++i){Sleep(100);game=FindWindowA(NULL,"Game Information");}
  if(!game){puts("no game information dialog");return 2;}
  PostMessageA(game,WM_COMMAND,IDOK,0);
 }
 Sleep(1500);
 printf("sent stars toggle/game command to%p\n",(void *)main);return 0;
}
