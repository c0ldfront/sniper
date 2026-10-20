#include "online_connection.h"
#include "../src/network_compression.h"
#include "../src/network_checksum.h"
#include "../src/network_lifecycle.h"
#include "../src/level.h"
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
/* Controllers retain endpoint provenance independently of mutable client hooks. */
typedef struct OnlineOwned {
    Sniper87OnlineConnection connection;
    struct OnlineOwned *next;
    char hostname[136],port[8];
} OnlineOwned;
static OnlineOwned *owned_connections;
static void json_string(FILE*f,const char*s){
    fputc('"',f);
    for(const unsigned char*p=(const unsigned char*)s;*p;p++){
        if(*p=='"'||*p=='\\'){fputc('\\',f);fputc(*p,f);}
        else if(*p<32||*p>=127)fprintf(f,"\\u%04x",*p);
        else fputc(*p,f);
    }
    fputc('"',f);
}
static int sidecar_path(char*out,const char*path){int n=snprintf(out,MAX_PATH,"%s.source.json",path);return n>0&&n<MAX_PATH;}
static int download_metadata(char output[MAX_PATH],const char*directory,
    SniperNetworkClient*client,uint8_t opcode,int has_expected,uint32_t expected,
    const uint8_t*bytes,size_t size){
    if(!GetTempFileNameA(directory,"met",0,output))return 0;
    FILE*f=fopen(output,"wb");if(!f){DeleteFileA(output);return 0;}
    const OnlineOwned*source=owned_connections;
    while(source && &source->connection.client!=client)source=source->next;
    SYSTEMTIME now;GetSystemTime(&now);
    fprintf(f,"{\n  \"format\": \"sniper87-download-v1\",\n  \"protocol\": \"Sniper v87 TCP bootstrap / reliable UDP\",\n  \"hostname\": ");
    if(source)json_string(f,source->hostname);else fputs("null",f);
    fputs(",\n  \"tcp_port\": ",f);if(source)json_string(f,source->port);else fputs("null",f);
    fprintf(f,",\n  \"request_opcode\": %u,\n  \"received_utc\": \"%04u-%02u-%02uT%02u:%02u:%02uZ\",\n  \"bytes\": %llu,\n  \"checksum_algorithm\": \"%s\",\n  \"actual_checksum\": \"%08lx\",\n  \"expected_checksum\": ",
        opcode,now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,
        (unsigned long long)size,opcode==12?"sum-byte-complete-four-byte-records-mod2^32":"sum-le32-complete-words-mod2^32",
        (unsigned long)(opcode==12?sniper_level_checksum(bytes,size):sniper_network_checksum(bytes,size)));
    if(has_expected)fprintf(f,"\"%08lx\"",(unsigned long)expected);else fputs("null",f);
    fputs("\n}\n",f);int ok=!ferror(f);if(fclose(f))ok=0;if(!ok)DeleteFileA(output);return ok;
}
/* Preserve the previous artifact AND its sidecar under the same unique stem. */
static int install_download(const char*directory,const char*temp,const char*metadata,const char*target){
    char side[MAX_PATH],backup[MAX_PATH],backup_side[MAX_PATH];
    if(!sidecar_path(side,target))return 0;
    int old_file=GetFileAttributesA(target)!=INVALID_FILE_ATTRIBUTES;
    int old_side=GetFileAttributesA(side)!=INVALID_FILE_ATTRIBUTES;
    if(old_file||old_side){
        if(!GetTempFileNameA(directory,"old",0,backup))return 0;
        DeleteFileA(backup);if(!sidecar_path(backup_side,backup))return 0;
        if(old_file&&!MoveFileA(target,backup))return 0;
        if(old_side&&!MoveFileA(side,backup_side)){if(old_file)MoveFileA(backup,target);return 0;}
    }
    if(!MoveFileA(temp,target))goto restore;
    if(MoveFileA(metadata,side))return 1;
    MoveFileA(target,temp);
restore:
    if(old_file)MoveFileA(backup,target);
    if(old_side)MoveFileA(backup_side,side);
    return 0;
}
static void pause_poll(void *unused){(void)unused;Sleep(1);}
static uint8_t *decompress(void*u,const uint8_t*p,size_t n,size_t*out){(void)u;if(n>INT32_MAX)return NULL;int32_t size=0;uint8_t*r=sniper_network_decompress(p,(int32_t)n,&size);if(r)*out=(size_t)size;return r;}
static int messages(Sniper87OnlineConnection*c){MSG m;while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){if(m.message==WM_QUIT){PostQuitMessage((int)m.wParam);c->cancelled=1;return 0;}if(!c->download_window||!IsDialogMessageA(c->download_window,&m)){TranslateMessage(&m);DispatchMessageA(&m);}}return !c->cancelled;}
static void write_at(void*u,int32_t offset,const uint8_t*p,size_t n){Sniper87OnlineConnection*c=u;DWORD written=0;if(offset<0||n>INT32_MAX||(uint64_t)(uint32_t)offset+n>INT32_MAX||SetFilePointer(c->download_file,offset,NULL,FILE_BEGIN)==INVALID_SET_FILE_POINTER||!WriteFile(c->download_file,p,(DWORD)n,&written,NULL)||written!=n)c->write_failed=1;else if(c->download_window)SendDlgItemMessageA(c->download_window,1011,PBM_SETPOS,(WPARAM)(((uint32_t)offset+n)/1024),0);}
static SniperNetworkClient *active(Sniper87OnlineConnection*c){return c->transfer_client?c->transfer_client:&c->client;}
static INT_PTR CALLBACK download_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    Sniper87OnlineConnection*c=(void*)GetWindowLongPtrA(w,DWLP_USER);
    if(msg==WM_INITDIALOG){c=(void*)lp;SetWindowLongPtrA(w,DWLP_USER,(LONG_PTR)c);c->download_window=w;SetDlgItemTextA(w,1012,c->level_path);SetTimer(w,1,50,NULL);return TRUE;}
    if(!c)return FALSE;
    if(msg==WM_CLOSE||(msg==WM_COMMAND&&(LOWORD(wp)==IDOK||LOWORD(wp)==IDCANCEL))){c->cancelled=1;(void)sniper_network_client_cancel_download(active(c));KillTimer(w,1);EndDialog(w,0);return TRUE;}
    if(msg==WM_TIMER){
        int rc=sniper_network_client_poll(active(c));
        SendDlgItemMessageA(w,1011,PBM_SETRANGE,0,MAKELPARAM(0,(uint16_t)(active(c)->download.total/1024)));
        if(rc<0||c->write_failed||!sniper_network_transport_alive(&active(c)->transport,1)){KillTimer(w,1);EndDialog(w,-1);return TRUE;}
        if(active(c)->download.done){KillTimer(w,1);EndDialog(w,1);return TRUE;}
        return TRUE;
    }
    if(msg==WM_DESTROY)c->download_window=NULL;
    return FALSE;
}
static uint8_t *load_file(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long length=ftell(f);if(length<0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*p=malloc(length?(size_t)length:1);if(!p){fclose(f);return NULL;}if(fread(p,1,(size_t)length,f)!=(size_t)length){free(p);fclose(f);return NULL;}fclose(f);*n=(size_t)length;return p;}
static int make_path(char*out,const char*dir,const char*name){int n=snprintf(out,MAX_PATH,"%s%s%s",dir,dir[0]&&dir[strlen(dir)-1]!='/'&&dir[strlen(dir)-1]!='\\'?"/":"",name);return n>0&&n<MAX_PATH;}
static int valid_name(const char*s){for(;*s;s++)if(!((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||*s=='.'||*s=='_'||*s=='-'))return 0;return 1;}
static int transfer(Sniper87OnlineConnection*c,const char*dir,const char*target,uint32_t checksum,int arena){
    char temp[MAX_PATH],metadata[MAX_PATH];if(!GetTempFileNameA(dir,"s87",0,temp))return 0;
    c->download_file=CreateFileA(temp,GENERIC_WRITE,0,NULL,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(c->download_file==INVALID_HANDLE_VALUE){DeleteFileA(temp);return 0;}
    c->cancelled=c->write_failed=0;
    int requested=arena?sniper_network_client_request_arenas(&c->client):sniper_network_client_request_map(&c->client);
    char old_label[MAX_PATH];memcpy(old_label,c->level_path,sizeof(old_label));if(target!=c->level_path)strcpy(c->level_path,target);
    INT_PTR result=requested?DialogBoxParamA(c->instance,MAKEINTRESOURCEA(149),c->owner,download_proc,(LPARAM)c):-1;
    memcpy(c->level_path,old_label,sizeof(old_label));CloseHandle(c->download_file);c->download_file=INVALID_HANDLE_VALUE;
    size_t size=0;uint8_t*bytes=result==1?load_file(temp,&size):NULL;
    uint32_t got=bytes?(arena?sniper_network_checksum(bytes,size):sniper_level_checksum(bytes,size)):0;
    int valid=bytes&&got==checksum;
    if(valid)valid=download_metadata(metadata,dir,&c->client,arena?18:12,1,checksum,bytes,size);
    free(bytes);
    if(!valid){DeleteFileA(temp);return 0;}
    int installed=install_download(dir,temp,metadata,target);
    if(!installed){DeleteFileA(temp);DeleteFileA(metadata);}
    return installed;
}
void sniper87_online_disconnect(Sniper87OnlineConnection*c){if(!c)return;if(c->client.transport.sender.connected)sniper_network_transport_shutdown(&c->client.transport,pause_poll,NULL);sniper_network_client_clear(&c->client);sniper_network_socket_close(&c->socket);if(c->download_file!=INVALID_HANDLE_VALUE)CloseHandle(c->download_file);free(c->level_bytes);OnlineOwned**at=&owned_connections;while(*at && &(*at)->connection!=c)at=&(*at)->next;if(*at)*at=(*at)->next;free(c);}
Sniper87OnlineConnection *sniper87_online_connect(HWND owner,HINSTANCE instance,const Sniper87ConnectionSettings*s,const Sniper87OnlineOptions*o){
    if(!s||!o)return NULL;
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);
    char*end=NULL;long port=strtol(s->port,&end,10);if(!end||*end||port<=0||port>65535)return NULL;
    const char*dir=o->cache_directory?o->cache_directory:".";
    OnlineOwned*allocation=calloc(1,sizeof(*allocation));if(!allocation)return NULL;allocation->next=owned_connections;owned_connections=allocation;snprintf(allocation->hostname,sizeof(allocation->hostname),"%s",s->host);snprintf(allocation->port,sizeof(allocation->port),"%s",s->port);Sniper87OnlineConnection*c=&allocation->connection;c->owner=owner;c->instance=instance;c->download_file=INVALID_HANDLE_VALUE;
    const char*failure="Could not connect to host";HCURSOR old=SetCursor(LoadCursorA(NULL,IDC_WAIT));BOOL enabled=owner?IsWindowEnabled(owner):FALSE;if(enabled)EnableWindow(owner,FALSE);
    if(!sniper_network_socket_open(&c->socket,s->host,(uint16_t)port))goto fail;
    SniperNetworkClientHost h={0};h.transport=(SniperTransportHost){&c->socket,sniper_network_socket_clock,sniper_network_socket_read,sniper_network_socket_send,sniper_network_socket_close,sniper_network_compress_callback,decompress};h.user=c;h.download_write=write_at;sniper_network_client_init(&c->client,&h);
    uint32_t start=GetTickCount()/10;
    while(GetTickCount()/10-start<150){size_t n;(void)sniper_network_transport_receive(&c->client.transport,&n);if(!messages(c)||!sniper_network_transport_alive(&c->client.transport,1))goto fail;Sleep(1);}
    int create=0;
    for(;;){
        if(!sniper_network_client_login(&c->client,s->player_name,s->password,create))goto fail;
        while(!c->client.login_valid){if(sniper_network_client_poll(&c->client)<0||!messages(c))goto fail;Sleep(1);}
        if(c->client.login.status==1){SetCursor(old);int answer=MessageBoxA(owner,"Unknown player -- continue as a new user?","Warning",MB_YESNO|MB_ICONQUESTION);SetCursor(LoadCursorA(NULL,IDC_WAIT));if(answer!=IDYES){c->cancelled=1;goto fail;}create=1;continue;}
        if(c->client.login.status==2){MessageBoxA(owner,"Invalid password for specified user","Error",MB_OK|MB_ICONEXCLAMATION);failure="Connection terminated.";goto fail;}
        if(c->client.login.status==3){MessageBoxA(owner,"You have been locked out of the system","Error",MB_OK|MB_ICONEXCLAMATION);failure="Connection terminated.";goto fail;}
        break;
    }
    if(c->client.login.version>87||(!o->bypass_exe_checksum&&c->client.login.exe_checksum!=o->exe_checksum)){char title[80];snprintf(title,sizeof(title),"Update version %d.%02d available!",c->client.login.version/100,c->client.login.version%100);SetCursor(old);if(MessageBoxA(owner,"Downloading from Web site recommended (www.vie.com/sniper) or download from here now (much slower).  Download from here?",title,MB_YESNO|MB_ICONQUESTION)==IDYES&&owner)PostMessageA(owner,WM_COMMAND,40013,0);c->cancelled=1;goto fail;}
    char basename[13];if(!sniper_login_map_name(basename,s->host,s->port)||!valid_name(basename)||!make_path(c->level_path,dir,basename)){failure="Unable to create the level cache path";goto fail;}
    c->level_bytes=load_file(c->level_path,&c->level_size);
    if(!c->level_bytes||sniper_level_checksum(c->level_bytes,c->level_size)!=c->client.login.map_checksum){
        free(c->level_bytes);c->level_bytes=NULL;
        MessageBoxA(owner,"It will be automatically downloaded now.","New Level Available",MB_OK|MB_ICONINFORMATION);
        if(!transfer(c,dir,c->level_path,c->client.login.map_checksum,0)){failure="Unable to download a valid level";goto fail;}
        c->level_bytes=load_file(c->level_path,&c->level_size);if(!c->level_bytes){failure="Unable to load the downloaded level";goto fail;}
    }
    char arena[MAX_PATH];if(!make_path(arena,dir,"arena.lst"))goto fail;
    if(sniper_network_checksum_file(arena)!=c->client.login.arena_checksum&&!transfer(c,dir,arena,c->client.login.arena_checksum,1)){failure="Unable to download a valid arena list";goto fail;}
    SetCursor(old);if(enabled)EnableWindow(owner,TRUE);return c;
fail:
    SetCursor(old);if(enabled)EnableWindow(owner,TRUE);if(!c->cancelled)MessageBoxA(owner,failure,!strcmp(failure,"Connection terminated.")?"Warning":"Error",MB_OK|MB_ICONEXCLAMATION);sniper87_online_disconnect(c);return NULL;
}
static void forward_event(void*u,const SniperGameEvent*e){Sniper87OnlineConnection*c=u;if(c->previous_host.event)c->previous_host.event(c->previous_host.event_user?c->previous_host.event_user:c->previous_host.user,e);}
static void forward_login(void*u,const SniperLoginReply*r){Sniper87OnlineConnection*c=u;if(c->previous_host.login_reply)c->previous_host.login_reply(c->previous_host.user,r);}
int sniper87_network_download(HWND owner,HINSTANCE instance,SniperNetworkClient*client,const char*path,uint8_t opcode){
    if(!client||!path||strlen(path)>=MAX_PATH||client->download_active)return -1;
    Sniper87OnlineConnection*c=calloc(1,sizeof(*c));if(!c)return -1;
    c->owner=owner;c->instance=instance;c->transfer_client=client;c->previous_host=client->host;c->download_file=INVALID_HANDLE_VALUE;strcpy(c->level_path,path);
    char full[MAX_PATH],*base=NULL,temp[MAX_PATH],metadata[MAX_PATH];DWORD fullsize=GetFullPathNameA(path,MAX_PATH,full,&base);
    if(!fullsize||fullsize>=MAX_PATH||!base){free(c);return -1;}
    *base=0;
    if(!GetTempFileNameA(full,"s87",0,temp)){free(c);return -1;}
    c->download_file=CreateFileA(temp,GENERIC_WRITE,0,NULL,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(c->download_file==INVALID_HANDLE_VALUE){DeleteFileA(temp);free(c);return -1;}
    client->host.user=c;client->host.event_user=c;client->host.event=forward_event;client->host.login_reply=forward_login;client->host.download_write=write_at;client->host.download_done=NULL;
    SniperGamePacket p={0};p.data[0]=opcode;p.length=p.reliable=p.copy=1;
    int sent=sniper_network_client_send(client,&p);memset(&client->download,0,sizeof(client->download));client->download_active=sent;
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);
    INT_PTR result=sent?DialogBoxParamA(instance,MAKEINTRESOURCEA(149),owner,download_proc,(LPARAM)c):-1;
    client->host=c->previous_host;client->download_active=0;CloseHandle(c->download_file);
    int outcome=-1;
    if(result==1&&!c->write_failed){
        size_t size=0;uint8_t*bytes=load_file(temp,&size);
        if(bytes&&download_metadata(metadata,full,client,opcode,0,0,bytes,size)){
            if(install_download(full,temp,metadata,path))outcome=1;
            else DeleteFileA(metadata);
        }
        free(bytes);
    }else if(c->cancelled)outcome=0;
    if(outcome!=1)DeleteFileA(temp);
    free(c);return outcome;
}
