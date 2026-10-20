#include "network_commands.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static uint16_t le16(const uint8_t *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int sniper87_network_score_parse(const uint8_t *p,size_t n,SniperPlayerScore **result,int *count){
 if(!p||!n||p[0]!=11||!result||!count)return 0;
 *result=NULL;*count=0;p++;n--;SniperPlayerScore *rows=NULL;
 while(n){SniperPlayerScore row={0};char *dest[2]={row.name,row.squad};
  for(int i=0;i<2;i++){const uint8_t *end=memchr(p,0,n);if(!end||end-p>=32)goto fail;size_t bytes=(size_t)(end-p)+1;memcpy(dest[i],p,bytes);p+=bytes;n-=bytes;}
  if(n<8||*count==INT_MAX)goto fail;
  row.wins=(int16_t)le16(p);row.losses=(int16_t)le16(p+2);row.points=(int32_t)le32(p+4);p+=8;n-=8;
  SniperPlayerScore *next=realloc(rows,((size_t)*count+1)*sizeof(*rows));if(!next)goto fail;rows=next;rows[(*count)++]=row;
 }*result=rows;return 1;
 fail:free(rows);*count=0;return 0;
}
int sniper87_network_squad_scores(SniperPlayerScore *players,int count,SniperSquadScore **result,int *out_count){
 if(!result||!out_count||count<0||(!players&&count))return 0;
 *result=NULL;*out_count=0;if(!count)return 1;
 SniperSquadScore *s=calloc((size_t)count,sizeof(*s));if(!s)return 0;
 int n=sniper87_build_squad_scores(players,count,s,count);
 if(n<0){free(s);return 0;}
 *result=s;*out_count=n;return 1;
}

int sniper87_network_squad_packet(uint8_t p[65],const char *name,const char *password){
 if(!p||!name||!password||strlen(name)>31||strlen(password)>31)return 0;
 while(*name==' ')name++;
 size_t n=strlen(name);while(n&&name[n-1]==' ')n--;if(!n)return 0;
 memset(p,0,65);p[0]=19;memcpy(p+1,name,n);memcpy(p+33,password,strlen(password));return 1;
}
#ifdef _WIN32
#include "squad_dialog.h"
#include "file_view_dialog.h"
#include "online_connection.h"
#include <commdlg.h>
#include <stdio.h>
static uint32_t command_clock(void *u){SniperNetworkClient *c=u;return c->host.transport.clock_ms?c->host.transport.clock_ms(c->host.transport.user):GetTickCount();}
static void command_event(void *u,const SniperGameEvent *e){SniperNetworkClient *c=u;if(c->host.event)c->host.event(c->host.event_user?c->host.event_user:c->host.user,e);}
static const uint8_t *wait_packet(SniperNetworkClient *c,uint8_t opcode,uint32_t timeout,size_t *n){
 uint32_t start=command_clock(c)/10;SniperGameReceiveHost h={c,command_clock,command_event,NULL};
 while(command_clock(c)/10-start<timeout){
  if(GetAsyncKeyState(VK_ESCAPE)&0x8000)return NULL;
  const uint8_t *p=sniper_network_transport_receive(&c->transport,n);
  c->game.clock_offset=(uint32_t)c->transport.receiver.clock_offset;
  if(p){if(*n&&p[0]==opcode)return p;if(!sniper_game_receive_packet(&c->game,&h,p,*n))return NULL;}
  else {if(!c->transport.sender.connected||c->transport.invalid_packet)return NULL;Sleep(1);}
 }return NULL;
}
int sniper87_network_command(HWND owner,HINSTANCE instance,SniperNetworkClient *c,unsigned command){
 if(command!=40012&&command!=40030&&command!=40031&&command!=40014&&command!=40013)return 0;
 if(!c||!c->transport.sender.connected)return 1;
 if(command==40031){Sniper87SquadCredentials credentials={{0},{0}};uint8_t packet[65];
  if(sniper87_squad_dialog(owner,instance,&credentials)!=IDOK||!sniper87_network_squad_packet(packet,credentials.name,credentials.password))return 1;
  if(!sniper_network_send_packet(&c->transport.sender,packet,65,1,1))return 1;
  size_t n;const uint8_t *p=wait_packet(c,19,3000,&n);if(p&&n>=2){
   static const char *text[]={"New squad has been formed.","Welcome to the squad.","Invalid password for this squad."};
   static const char *title[]={"New Squad","Membership Approved","Membership Denied"};
   if(p[1]<3)MessageBoxA(owner,text[p[1]],title[p[1]],MB_OK);
  }return 1;
 }
 if(command==40012||command==40030){uint8_t request=10;HCURSOR previous=SetCursor(LoadCursorA(NULL,IDC_WAIT));
  size_t n=0;const uint8_t *p=NULL;if(sniper_network_send_packet(&c->transport.sender,&request,1,1,1))p=wait_packet(c,11,4000,&n);
  SniperPlayerScore *players=NULL;int count=0;int ok=p&&sniper87_network_score_parse(p,n,&players,&count);SetCursor(previous);
  if(ok&&count){if(command==40012)sniper87_player_scores_dialog(owner,instance,players,count);
   else {SniperSquadScore *squads=NULL;int squad_count=0;if(sniper87_network_squad_scores(players,count,&squads,&squad_count)&&squad_count)sniper87_squad_scores_dialog(owner,instance,squads,squad_count);free(squads);}}
  free(players);return 1;
 }
 if(command==40014){int result=sniper87_network_download(owner,instance,c,"news.txt",13);if(result>=0)sniper87_file_view_dialog(owner,instance,"news.txt");return 1;}
 char path[MAX_PATH];snprintf(path,sizeof(path),"Sniper%d.EXE",c->login.version);OPENFILENAMEA ofn={0};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=owner;ofn.lpstrFile=path;ofn.nMaxFile=MAX_PATH;ofn.lpstrFilter="Executable Files\0*.EXE\0All Files\0*.*\0\0";ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
 if(GetSaveFileNameA(&ofn)&&sniper87_network_download(owner,instance,c,path,11)==1){char text[MAX_PATH+80];snprintf(text,sizeof(text),"You should exit the application now and run '%s'.",path);MessageBoxA(owner,text,"Instructions",MB_ICONINFORMATION);}
 return 1;
}
#endif
