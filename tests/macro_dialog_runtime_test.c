#include "macro_dialog.h"
#include <stdio.h>
#include <string.h>
static char macros[8][82];static int mode,failed;static const int ids[8]={1020,1021,1022,1026,1024,1025,1023,1027};
static VOID CALLBACK drive(HWND ignored,UINT message,UINT_PTR timer,DWORD time) {
    (void)ignored;(void)message;(void)time;
    HWND dialog=FindWindowA(NULL,"Message Hot Keys");if(!dialog)return;
    KillTimer(NULL,timer);
    for(int i=0;i<8;++i){char text[128],want[32];GetDlgItemTextA(dialog,ids[i],text,sizeof text);snprintf(want,sizeof want,"initial%d",i);if(strcmp(text,want))failed=1;memset(text,'A'+i,100);text[100]=0;SetDlgItemTextA(dialog,ids[i],text);}
    if(mode==2)SendMessageA(dialog,WM_CLOSE,0,0);else SendMessageA(dialog,WM_COMMAND,mode?IDCANCEL:IDOK,0);
}
int main(void) {
    for(mode=0;mode<3;++mode){
        for(int i=0;i<8;++i)snprintf(macros[i],82,"initial%d",i);
        SetTimer(NULL,1,50,drive);INT_PTR result=sniper87_macro_dialog(NULL,GetModuleHandle(NULL),macros);
        if(result!=(mode?0:1))failed=1;
        for(int i=0;i<8;++i){char want[82];if(!mode){memset(want,'A'+i,81);want[81]=0;}else snprintf(want,82,"initial%d",i);if(strcmp(macros[i],want))failed=1;}
    }
    printf("%s real template152 initialization,all8 edits,81-character truncation,OK,Cancel,titlebar close\n",failed?"FAIL":"PASS");return failed;
}
