#include "network_file_receive.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
static int path(char*out,const char*directory,const char*name){int n=snprintf(out,MAX_PATH,"%s\\%s",directory,name);return n>0&&n<MAX_PATH;}
static int sidecar(char*out,const char*artifact){int n=snprintf(out,MAX_PATH,"%s.source.json",artifact);return n>0&&n<MAX_PATH;}
static int basename_ok(const char*s,size_t n){
    if(!n||n>15||s[0]=='.'||s[n-1]=='.'||s[n-1]==' ')return 0;
    for(size_t i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='.'||s[i]=='_'||s[i]=='-'||s[i]==' '))return 0;
    char stem[16];size_t z=0;while(z<n&&s[z]!='.'){stem[z]=(char)toupper((unsigned char)s[z]);z++;}while(z&&stem[z-1]==' ')z--;stem[z]=0;
    if(!strcmp(stem,"CON")||!strcmp(stem,"PRN")||!strcmp(stem,"AUX")||!strcmp(stem,"NUL"))return 0;
    if(z==4&&(!memcmp(stem,"COM",3)||!memcmp(stem,"LPT",3))&&stem[3]>='1'&&stem[3]<='9')return 0;
    return 1;
}
static void abort_stage(SniperNetworkFileReceive*s){if(s->file!=INVALID_HANDLE_VALUE)CloseHandle(s->file);s->file=INVALID_HANDLE_VALUE;if(s->temporary[0])DeleteFileA(s->temporary);s->temporary[0]=0;s->active=0;}
void sniper_network_file_receive_clear(SniperNetworkFileReceive*s){if(s)abort_stage(s);}
int sniper_network_file_receive_init(SniperNetworkFileReceive*s,const char*host,const char*port){
    if(!s||!host||!port||strlen(host)>=sizeof(s->hostname)||strlen(port)>=sizeof(s->port))return 0;
    memset(s,0,sizeof(*s));s->file=INVALID_HANDLE_VALUE;DWORD n=GetCurrentDirectoryA(MAX_PATH,s->directory);if(!n||n>=MAX_PATH)return 0;strcpy(s->hostname,host);strcpy(s->port,port);return 1;
}
static void json(FILE*f,const char*s){fputc('"',f);for(const unsigned char*p=(const unsigned char*)s;*p;p++){if(*p=='"'||*p=='\\'){fputc('\\',f);fputc(*p,f);}else if(*p<32||*p>=127)fprintf(f,"\\u%04x",*p);else fputc(*p,f);}fputc('"',f);}
static int metadata(SniperNetworkFileReceive*s,char*out){
    if(!GetTempFileNameA(s->directory,"met",0,out))return 0;
    FILE*f=fopen(out,"wb");if(!f){DeleteFileA(out);return 0;}
    SYSTEMTIME now;GetSystemTime(&now);
    fputs("{\n  \"format\": \"sniper87-server-file-v1\",\n  \"protocol\": \"Sniper v87 opcode18\",\n  \"hostname\": ",f);json(f,s->hostname);
    fputs(",\n  \"tcp_port\": ",f);json(f,s->port);fputs(",\n  \"wire_filename\": ",f);json(f,s->name);
    fprintf(f,",\n  \"received_utc\": \"%04u-%02u-%02uT%02u:%02u:%02uZ\",\n  \"bytes\": %llu,\n  \"checksum_algorithm\": \"CRC-32/ISO-HDLC\",\n  \"actual_checksum\": \"%08lx\",\n  \"expected_checksum\": null\n}\n",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,(unsigned long long)s->bytes,(unsigned long)(s->crc^UINT32_MAX));
    int ok=!ferror(f);if(fclose(f))ok=0;if(!ok)DeleteFileA(out);return ok;
}
static int commit(SniperNetworkFileReceive*s){
    char target[MAX_PATH],meta[MAX_PATH],staged_meta[MAX_PATH],backup[MAX_PATH],backup_meta[MAX_PATH];
    if(!path(target,s->directory,s->name)||!sidecar(meta,target)||!metadata(s,staged_meta))return 0;
    DWORD attrs=GetFileAttributesA(target);int old=attrs!=INVALID_FILE_ATTRIBUTES,oldmeta=GetFileAttributesA(meta)!=INVALID_FILE_ATTRIBUTES;
    /* A directory cannot be replaced by a server file. Reparse points are moved
       as directory entries rather than followed, but reject them explicitly. */
    if(old&&(attrs&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))goto fail;
    if(old||oldmeta){
        if(!GetTempFileNameA(s->directory,"old",0,backup))goto fail;
        DeleteFileA(backup);if(!sidecar(backup_meta,backup))goto fail;
        if(old&&!MoveFileA(target,backup))goto fail;
        if(oldmeta&&!MoveFileA(meta,backup_meta)){if(old)MoveFileA(backup,target);goto fail;}
    }
    if(!MoveFileA(s->temporary,target))goto restore;
    if(!MoveFileA(staged_meta,meta)){MoveFileA(target,s->temporary);goto restore;}
    s->temporary[0]=0;strcpy(s->committed,target);s->received++;return 1;
restore:
    if(old)MoveFileA(backup,target);
    if(oldmeta)MoveFileA(backup_meta,meta);
fail:
    DeleteFileA(staged_meta);return 0;
}
int sniper_network_file_receive_event(SniperNetworkFileReceive*s,const SniperGameEvent*e){
    if(!s||!e)return -1;
    if(e->kind==SNIPER_NET_FILE_OPEN){
        if(s->active){abort_stage(s);s->failed=1;return -1;}
        s->failed=0;s->bytes=0;s->crc=UINT32_MAX;
        const uint8_t*z=e->data&&e->size&&e->size<=16?memchr(e->data,0,e->size):NULL;
        if(!z||!basename_ok((const char*)e->data,(size_t)(z-e->data))||!e->data2||e->size2!=3||memcmp(e->data2,"wb",3)){s->failed=1;return -1;}
        memcpy(s->name,e->data,(size_t)(z-e->data)+1);
        if(!GetTempFileNameA(s->directory,"s87",0,s->temporary)){s->failed=1;return -1;}
        s->file=CreateFileA(s->temporary,GENERIC_WRITE,0,NULL,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(s->file==INVALID_HANDLE_VALUE){abort_stage(s);s->failed=1;return -1;}
        s->active=1;return 1;
    }
    if(e->kind==SNIPER_NET_FILE_WRITE){
        DWORD wrote=0;
        if(!s->active||s->failed||e->size>INT32_MAX||(!e->data&&e->size)||e->args[1]!=(int32_t)e->size||e->args[2]!=1||s->bytes+e->size>INT32_MAX||!WriteFile(s->file,e->data,(DWORD)e->size,&wrote,NULL)||wrote!=e->size){abort_stage(s);s->failed=1;return -1;}
        for(size_t i=0;i<e->size;i++){s->crc^=e->data[i];for(int bit=0;bit<8;bit++)s->crc=(s->crc>>1)^((s->crc&1)?UINT32_C(0xedb88320):0);}
        s->bytes+=e->size;return 1;
    }
    if(e->kind==SNIPER_NET_FILE_CLOSE){
        if(!s->active||s->failed){s->failed=1;return -1;}
        int ok=FlushFileBuffers(s->file)!=0;if(!CloseHandle(s->file))ok=0;s->file=INVALID_HANDLE_VALUE;
        if(ok)ok=commit(s);
        abort_stage(s);s->failed=!ok;return ok?1:-1;
    }
    return 0;
}
