/* Production command controller with local dialog/transport boundaries; no network. */
#include <windows.h>
#include <commdlg.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../platform/network_commands.h"
#include "../platform/squad_dialog.h"
#include "../platform/online_connection.h"
static int boxes,players_shown,squads_shown,forwarded,downloads,phase,mode;
static uint8_t sent[128];static int sent_size;static DWORD clock_value;
static int WINAPI message_box(HWND w,LPCSTR text,LPCSTR title,UINT type){(void)w;(void)type;assert(!strcmp(text,"Welcome to the squad."));assert(!strcmp(title,"Membership Approved"));boxes++;return IDOK;}
static BOOL WINAPI save_dialog(LPOPENFILENAMEA p){(void)p;return FALSE;}
#undef _WIN32
#include "../platform/score_dialog.c"
#define _WIN32 1
#define MessageBoxA message_box
#define GetSaveFileNameA save_dialog
#include "../platform/network_commands.c"
INT_PTR sniper87_squad_dialog(HWND w,HINSTANCE i,Sniper87SquadCredentials *c){(void)w;(void)i;strcpy(c->name,"  Test  ");strcpy(c->password,"secret");return IDOK;}
INT_PTR sniper87_player_scores_dialog(HWND w,HINSTANCE i,const SniperPlayerScore *p,int n){(void)w;(void)i;assert(n==2&&p[0].wins==1&&p[1].points==6);players_shown++;return IDOK;}
INT_PTR sniper87_squad_scores_dialog(HWND w,HINSTANCE i,const SniperSquadScore *p,int n){(void)w;(void)i;assert(n==1&&p[0].members==2&&p[0].points==9);squads_shown++;return IDOK;}
INT_PTR sniper87_file_view_dialog(HWND w,HINSTANCE i,const char *p){(void)w;(void)i;assert(!strcmp(p,"news.txt"));return IDOK;}
int sniper87_network_download(HWND w,HINSTANCE i,SniperNetworkClient *c,const char *p,uint8_t opcode){(void)w;(void)i;(void)c;assert(opcode==13&&!strcmp(p,"news.txt"));downloads++;return 1;}
int sniper_network_send_packet(SniperNetworkSend *s,const uint8_t *p,int32_t n,int reliable,int copy){(void)s;assert(reliable==1&&copy==1&&n<128);memcpy(sent,p,(size_t)n);sent_size=n;phase=0;return 1;}
const uint8_t *sniper_network_transport_receive(SniperNetworkTransport *t,size_t *n){(void)t;
 static const uint8_t game[]={99};static const uint8_t squad[]={19,1};
 static const uint8_t scores[]={11,'a',0,'S',0,1,0,2,0,3,0,0,0,'b',0,'S',0,4,0,5,0,6,0,0,0};
 if(mode==2)return NULL;
 if(!phase++){*n=1;return game;}
 *n=mode?sizeof(scores):sizeof(squad);return mode?scores:squad;
}
int sniper_game_receive_packet(SniperGameReceive *s,const SniperGameReceiveHost *h,const uint8_t *p,size_t n){(void)s;assert(n==1&&p[0]==99);SniperGameEvent e={0};h->event(h->user,&e);return 1;}
static void event_sink(void *u,const SniperGameEvent *e){(void)e;assert(u==&forwarded);forwarded++;}
static uint32_t clock_ms(void *u){(void)u;clock_value+=mode==2?10000:10;return clock_value;}
int main(void){SniperNetworkClient c={0};c.transport.sender.connected=1;c.host.transport.clock_ms=clock_ms;c.host.event=event_sink;c.host.event_user=&forwarded;
 assert(sniper87_network_command(NULL,NULL,&c,40031));assert(sent_size==65&&sent[0]==19&&!strcmp((char*)sent+1,"Test")&&!strcmp((char*)sent+33,"secret")&&boxes==1&&forwarded==1);
 mode=1;assert(sniper87_network_command(NULL,NULL,&c,40012));assert(players_shown==1&&sent_size==1&&sent[0]==10&&forwarded==2);
 assert(sniper87_network_command(NULL,NULL,&c,40030));assert(squads_shown==1&&forwarded==3);
 assert(sniper87_network_command(NULL,NULL,&c,40014));assert(downloads==1);
 mode=2;assert(sniper87_network_command(NULL,NULL,&c,40012));assert(players_shown==1);assert(sniper87_network_command(NULL,NULL,&c,40031));assert(boxes==1);
 assert(!sniper87_network_command(NULL,NULL,&c,12345));c.transport.sender.connected=0;assert(sniper87_network_command(NULL,NULL,&c,40031));assert(boxes==1);
 puts("PASS production network command dispatch: squad65-byte request/reply, variable score wire, aggregate dialog, news download,30/40s deadlines, gameplay forwarding, disconnected guard");return 0;}
