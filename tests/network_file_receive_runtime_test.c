#include "../platform/network_file_receive.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static SniperGameEvent open_event(const char*n){SniperGameEvent e={0};e.kind=SNIPER_NET_FILE_OPEN;e.data=(const uint8_t*)n;e.size=strlen(n)+1;e.data2=(const uint8_t*)"wb";e.size2=3;return e;}
static SniperGameEvent write_event(const char*b,size_t n){SniperGameEvent e={0};e.kind=SNIPER_NET_FILE_WRITE;e.data=(const uint8_t*)b;e.size=n;e.args[1]=(int32_t)n;e.args[2]=1;return e;}
static SniperGameEvent close_event(void){SniperGameEvent e={0};e.kind=SNIPER_NET_FILE_CLOSE;return e;}
static void save(const char*n,const char*b){FILE*f=fopen(n,"wb");assert(f);assert(fwrite(b,1,strlen(b),f)==strlen(b));assert(!fclose(f));}
static void matches(const char*n,const char*b){char out[100]={0};FILE*f=fopen(n,"rb");assert(f);assert(fread(out,1,99,f)==strlen(b)&&!strcmp(out,b));fclose(f);}
static int events;
static void bridge(void*u,const SniperGameEvent*e){SniperNetworkFileReceive*s=u;if(e->kind==SNIPER_NET_CHAT)return;assert(sniper_network_file_receive_event(s,e)==1);events++;}
int main(void){CreateDirectoryA("files",NULL);assert(SetCurrentDirectoryA("files"));SniperNetworkFileReceive s;assert(sniper_network_file_receive_init(&s,"127.0.0.1","382"));
    save("fixture.txt","previous bytes");save("fixture.txt.source.json","previous provenance");SniperGameEvent e=open_event("fixture.txt");assert(sniper_network_file_receive_event(&s,&e)==1);matches("fixture.txt","previous bytes");e=write_event("1234",4);assert(sniper_network_file_receive_event(&s,&e)==1);matches("fixture.txt","previous bytes");e=write_event("56789",5);assert(sniper_network_file_receive_event(&s,&e)==1);e=close_event();assert(sniper_network_file_receive_event(&s,&e)==1);assert(s.received==1&&s.bytes==9);matches("fixture.txt","123456789");
    const char*bad[]={"../outside","..\\outside","/absolute","C:relative","a/b","a\\b","x:y","CON","con.txt","CON .txt","AUX","NUL","LPT1.log","COM9",".hidden","trailing.","trailing ","","a\n","abcdefghijklmnop"};
    for(size_t i=0;i<sizeof(bad)/sizeof(*bad);i++){e=open_event(bad[i]);assert(sniper_network_file_receive_event(&s,&e)==-1);e=write_event("bad",3);assert(sniper_network_file_receive_event(&s,&e)==-1);e=close_event();assert(sniper_network_file_receive_event(&s,&e)==-1);}matches("fixture.txt","123456789");
    e=open_event("unfinished");assert(sniper_network_file_receive_event(&s,&e)==1);e=write_event("partial",7);assert(sniper_network_file_receive_event(&s,&e)==1);sniper_network_file_receive_clear(&s);assert(GetFileAttributesA("unfinished")==INVALID_FILE_ATTRIBUTES);
    e=open_event("double");assert(sniper_network_file_receive_event(&s,&e)==1);assert(sniper_network_file_receive_event(&s,&e)==-1);assert(GetFileAttributesA("double")==INVALID_FILE_ATTRIBUTES);
    CreateDirectoryA("directory",NULL);e=open_event("directory");assert(sniper_network_file_receive_event(&s,&e)==1);e=close_event();assert(sniper_network_file_receive_event(&s,&e)==-1);assert(GetFileAttributesA("directory")&FILE_ATTRIBUTE_DIRECTORY);
    uint8_t packet[22]={18};memcpy(packet+1,"decoder.bin",12);memcpy(packet+17,"bytes",5);SniperGameReceive game={0};SniperGameReceiveHost host={&s,NULL,bridge,NULL};assert(sniper_game_receive_packet(&game,&host,packet,sizeof(packet)));assert(events==3);matches("decoder.bin","bytes");sniper_network_file_receive_clear(&s);
    puts("PASS opcode18 production file sink: native decoder events, staged CLOSE-only commit, CRC/source sidecar, prior artifact+sidecar backup, path/device rejection, abort/error cleanup");return 0;
}
