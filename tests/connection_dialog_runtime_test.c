#include "connection_dialog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char *field[7];static int failed;
static VOID CALLBACK drive(HWND w,UINT m,UINT_PTR timer,DWORD t) {
    (void)w;(void)m;(void)t;HWND d=FindWindowA(NULL,"Connection Information");if(!d)return;
    KillTimer(NULL,timer);SetDlgItemTextA(d,1008,field[0]);SetDlgItemTextA(d,1007,"password");SetDlgItemTextA(d,1034,field[1]);
    SendMessageA(d,WM_COMMAND,atoi(field[2])?1033:IDOK,0);
}
int main(void) {
    FILE *f=fopen("arena.lst","wb");if(!f)return 1;fputs("Warzone,205.214.55.45,382\r\n\r\n",f);fclose(f);
    Sniper87ArenaEntry entries[32];size_t count=sniper87_connection_read_arenas("arena.lst",entries);
    if(count!=1 || strcmp(entries[0].name,"Warzone") || strcmp(entries[0].port,"382"))return 2;
    f=fopen("connection-oracle.tsv","rb");if(!f)return 3;char line[1024];int cases=0;
    while(fgets(line,sizeof line,f)) {
        line[strcspn(line,"\r\n")]=0;field[0]=line;
        for(int i=1;i<7;++i){char *tab=strchr(field[i-1],'\t');if(!tab)return 4;*tab=0;field[i]=tab+1;}
        Sniper87ConnectionSettings state={0};strcpy(state.player_name,"initial");strcpy(state.arena,field[1]);
        if(atoi(field[3])<0){if(sniper87_connection_resolve(&state,entries,count)!=1)failed=1;}
        else {
            SetTimer(NULL,1,40,drive);INT_PTR result=sniper87_connection_dialog(NULL,GetModuleHandle(NULL),&state);
            if(result!=atoi(field[3]) || strcmp(state.player_name,field[4]) || strcmp(state.password,"password"))failed=1;
        }
        if(strcmp(state.host,field[5]) || strcmp(state.port,field[6])){printf("mismatch%d host%s port%s\n",cases,state.host,state.port);failed=1;}++cases;
    }
    fclose(f);DeleteFileA("arena.lst");printf("%s%d original-oracle cases: actual resource136 dialog,quick-start,case-insensitive named arena,manual/default/empty port,name trimming quirk;invalid host resolver\n",failed?"FAIL":"PASS",cases);return failed;
}
