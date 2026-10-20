/* Original40dbf0..40e0fb,resource136. */
#include "connection_dialog.h"
#include <stdio.h>
#include <string.h>
static void copy_text(char *dst,size_t cap,const char *src) {
    size_t n=strlen(src);if(n>=cap)n=cap-1;memmove(dst,src,n);dst[n]=0;
}
size_t sniper87_connection_read_arenas(const char *path,Sniper87ArenaEntry entries[32]) {
    memset(entries,0,sizeof(*entries)*32);FILE *f=fopen(path,"rt");if(!f)return 0;
    char line[256];size_t count=0;
    while(count<32 && fgets(line,sizeof line,f)) {
        if(strlen(line)<=10)continue;
        const char *p=line;char *fields[]={entries[count].name,entries[count].host,entries[count].port};
        const size_t caps[]={80,64,8};
        for(int field=0;field<3;++field){size_t n=0;
            while((signed char)*p>=32 && *p!=','){if(n+1<caps[field])fields[field][n++]=*p;++p;}
            fields[field][n]=0;if(*p!=',')break;++p;
        }
        ++count;
    }
    fclose(f);return count;
}
int sniper87_connection_resolve(Sniper87ConnectionSettings *s,const Sniper87ArenaEntry *entries,size_t count) {
    for(size_t i=0;i<count;++i)if(_stricmp(s->arena,entries[i].name)==0){
        copy_text(s->host,sizeof s->host,entries[i].host);copy_text(s->port,sizeof s->port,entries[i].port);return 0;
    }
    char text[128];copy_text(text,sizeof text,s->arena);char *comma=strchr(text,',');
    if(comma){*comma++=0;copy_text(s->port,sizeof s->port,comma);}else copy_text(s->port,sizeof s->port,"382");
    copy_text(s->host,sizeof s->host,text);return strchr(s->host,'.')?0:1;
}
typedef struct {Sniper87ConnectionSettings *settings;Sniper87ArenaEntry entries[32];size_t count;HWND owner;} ConnectionDialog;
static INT_PTR CALLBACK connection_proc(HWND dialog,UINT msg,WPARAM wp,LPARAM lp) {
    ConnectionDialog *ctx=(ConnectionDialog *)GetWindowLongPtrA(dialog,DWLP_USER);
    switch(msg){
    case WM_INITDIALOG:{
        ctx=(ConnectionDialog *)lp;SetWindowLongPtrA(dialog,DWLP_USER,(LONG_PTR)ctx);
        Sniper87ConnectionSettings *s=ctx->settings;
        SetWindowTextA(GetDlgItem(dialog,1008),s->player_name);SetWindowTextA(GetDlgItem(dialog,1007),s->password);
        if(!GetPrivateProfileIntA("Dialog","PlayerShipPicked",0,"sniper.ini"))ShowWindow(GetDlgItem(dialog,1033),SW_HIDE);
        ctx->count=sniper87_connection_read_arenas("arena.lst",ctx->entries);HWND combo=GetDlgItem(dialog,1034);
        for(size_t i=0;i<ctx->count;++i)SendMessageA(combo,CB_ADDSTRING,0,(LPARAM)ctx->entries[i].name);
        if(!s->arena[0] && ctx->count)copy_text(s->arena,sizeof s->arena,ctx->entries[0].name);
        if(SendMessageA(combo,CB_SELECTSTRING,(WPARAM)-1,(LPARAM)s->arena)==CB_ERR)SetWindowTextA(combo,s->arena);
        return TRUE;}
    case WM_CLOSE:EndDialog(dialog,(INT_PTR)wp);return TRUE;
    case WM_COMMAND:
        if(LOWORD(wp)==IDCANCEL){PostMessageA(dialog,WM_CLOSE,0,0);return TRUE;}
        if(LOWORD(wp)==IDOK || LOWORD(wp)==1033){
            Sniper87ConnectionSettings *s=ctx->settings;GetWindowTextA(GetDlgItem(dialog,1008),s->player_name,32);
            /* Original memmove omits NUL, so removing a leading space repeats
             * the final byte. Preserve that quirk for nonblank names; reject
             * all-control/space input instead of its nonterminating loop. */
            char *first=s->player_name;while(*first && (signed char)*first<=32)++first;
            if(!*first)s->player_name[0]=0;
            else while((signed char)s->player_name[0]<=32)
                memmove(s->player_name,s->player_name+1,strlen(s->player_name+1));
            if(!s->player_name[0]){MessageBoxA(ctx->owner,"Player name must be entered.","Error",MB_ICONINFORMATION);return TRUE;}
            size_t n=strlen(s->player_name);while(n && (signed char)s->player_name[n-1]<=32)s->player_name[--n]=0;
            GetWindowTextA(GetDlgItem(dialog,1007),s->password,32);GetWindowTextA(GetDlgItem(dialog,1034),s->arena,128);
            if(sniper87_connection_resolve(s,ctx->entries,ctx->count)){MessageBoxA(ctx->owner,"Invalid arena specified","Error",MB_ICONINFORMATION);return TRUE;}
            PostMessageA(dialog,WM_CLOSE,LOWORD(wp)==1033?2:1,0);
        }
        return TRUE;
    default:return FALSE;
    }
}
INT_PTR sniper87_connection_dialog(HWND owner,HINSTANCE instance,Sniper87ConnectionSettings *settings) {
    if(!settings)return -1;
    ConnectionDialog ctx={0};ctx.settings=settings;ctx.owner=owner;
    return DialogBoxParamA(instance,MAKEINTRESOURCEA(136),owner,connection_proc,(LPARAM)&ctx);
}
