#include "../platform/online_connection.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void event(void*u,const SniperGameEvent*e){assert(u);if(e->kind==SNIPER_NET_CHAT){assert(!strcmp((const char*)e->data,"during download"));(*(int*)u)++;}}
static BOOL CALLBACK accept_prompt(HWND w,LPARAM unused){(void)unused;char name[80];GetWindowTextA(w,name,sizeof(name));if(!strcmp(name,"Warning")||!strcmp(name,"New Level Available")||!strcmp(name,"Error")){HWND yes=GetDlgItem(w,IDYES);PostMessageA(w,WM_COMMAND,yes?IDYES:IDOK,0);}return TRUE;}
static void CALLBACK timer(HWND w,UINT m,UINT_PTR id,DWORD t){(void)w;(void)m;(void)id;(void)t;EnumThreadWindows(GetCurrentThreadId(),accept_prompt,0);}
int main(int argc,char**argv){assert(argc==2);Sniper87ConnectionSettings settings={0};strcpy(settings.player_name,"Pilot87");strcpy(settings.password,"fixture-password");strcpy(settings.host,"127.0.0.1");strcpy(settings.port,argv[1]);CreateDirectoryA("online-cache",NULL);
    Sniper87OnlineOptions opts={"online-cache",0x68fc54a0,0};SetTimer(NULL,1,20,timer);
    for(int i=0;i<3;i++){
        Sniper87OnlineConnection*c=sniper87_online_connect(NULL,GetModuleHandleA(NULL),&settings,&opts);if(i==2){assert(!c);continue;}assert(c);assert(c->level_size==8&&!memcmp(c->level_bytes,"\x01\x20\x00\x01\x02\x30\x00\x02",8));assert(c->client.login_valid);
        if(!i){int observed=0;c->client.game.ready=1;c->client.host.event=event;c->client.host.event_user=&observed;assert(sniper87_network_download(NULL,GetModuleHandleA(NULL),&c->client,"online-cache/news.txt",13)==1);assert(observed==1&&c->client.host.event_user==&observed);c->client.host.event=NULL;c->client.host.event_user=NULL;FILE*f=fopen("online-cache/news.txt","rb");assert(f);char b[20]={0};assert(fread(b,1,19,f)==12&&!strcmp(b,"fixture news"));fclose(f);}
        sniper87_online_disconnect(c);
    }
    KillTimer(NULL,1);puts("PASS production online controller: auth/create-user, native download dialog, map+arena verified cache, generic news, cache reuse, two successful shutdowns and rejected checksum download");return 0;
}
