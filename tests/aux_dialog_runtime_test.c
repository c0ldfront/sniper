#include "squad_dialog.h"
#include "file_view_dialog.h"
#include <stdio.h>
#include <string.h>
static int mode,failed;
static VOID CALLBACK drive(HWND w,UINT m,UINT_PTR timer,DWORD t) {
    (void)w;(void)m;(void)t;HWND d=FindWindowA(NULL,mode<2?"Join Squad":"View File");if(!d)return;KillTimer(NULL,timer);
    if(mode<2){char buf[64];GetDlgItemTextA(d,1036,buf,sizeof buf);if(buf[0])failed=1;GetDlgItemTextA(d,1037,buf,sizeof buf);if(buf[0])failed=1;SetDlgItemTextA(d,1036,"abcdefghijklmnopqrstuvwxyz0123456789");SetDlgItemTextA(d,1037,"secret");SendMessageA(d,WM_COMMAND,mode?IDCANCEL:IDOK,0);}
    else {int n=GetWindowTextLengthA(GetDlgItem(d,1018));if(n!=(mode==2?0x7f00:0))failed=1;MoveWindow(d,30,40,350,240,TRUE);RECT a;GetClientRect(d,&a);RECT b;GetWindowRect(GetDlgItem(d,1018),&b);if(b.right-b.left!=a.right || b.bottom-b.top!=a.bottom)failed=1;SendMessageA(d,WM_COMMAND,IDOK,0);}
}
int main(void) {
    for(mode=0;mode<2;++mode){Sniper87SquadCredentials s={"original","originalpass"};SetTimer(NULL,1,50,drive);INT_PTR r=sniper87_squad_dialog(NULL,GetModuleHandle(NULL),&s);if(r!=(mode?0:1))failed=1;if(strcmp(s.name,mode?"original":"abcdefghijklmnopqrstuvwxyz01234") || strcmp(s.password,mode?"originalpass":"secret"))failed=1;}
    FILE *f=fopen("view-fixture.txt","wb");if(!f)return 2;for(int i=0;i<40000;++i)fputc('x',f);fclose(f);
    for(mode=2;mode<4;++mode){SetTimer(NULL,1,50,drive);if(sniper87_file_view_dialog(NULL,GetModuleHandle(NULL),mode==2?"view-fixture.txt":"absent.txt")!=0)failed=1;}
    if(GetPrivateProfileIntA("ViewFile","X",-1,"sniper.ini")!=30 || GetPrivateProfileIntA("ViewFile","Width",-1,"sniper.ini")!=350)failed=1;
    DeleteFileA("view-fixture.txt");printf("%s actual174 squad blank init/truncation/OK/Cancel;150 viewer read cap/missing file/resize/profile persistence\n",failed?"FAIL":"PASS");return failed;
}
