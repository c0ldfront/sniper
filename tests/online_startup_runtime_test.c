#include <windows.h>
#include <stdio.h>
static int WINAPI fixture_message(HWND w,LPCSTR text,LPCSTR title,UINT flags){(void)w;(void)flags;fprintf(stderr,"MODAL %s: %s\n",title,text);ExitProcess(99);return IDOK;}
#define MessageBoxA fixture_message
#include "../platform/startup.c"
#include <assert.h>
static char peer_arena[128];
static VOID CALLBACK accept_dialog(HWND w,UINT m,UINT_PTR timer,DWORD time){(void)m;(void)time;HWND d=GetLastActivePopup(w);if(d==w)return;if(GetDlgItem(d,1008)){SetDlgItemTextA(d,1008,"Online42");SetDlgItemTextA(d,1007,"local-fixture");SetDlgItemTextA(d,1034,peer_arena);}KillTimer(w,timer);PostMessageA(d,WM_COMMAND,IDOK,0);}
static void drain(void){MSG m;while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageA(&m);}}
static int voice_shift;
static int32_t held(void*u,int32_t key){(void)u;return voice_shift?(key==VK_SHIFT?0x8000:0):(key==VK_UP||key==VK_CONTROL?0x8000:0);}
int main(int argc,char**argv){assert(argc==2);setvbuf(stdout,NULL,_IONBF,0);snprintf(peer_arena,sizeof(peer_arena),"127.0.0.1,%s",argv[1]);instance=GetModuleHandleA(NULL);InitCommonControls();sniper_options_default(&options);options.value[SNIPER_SOUND]=1;
 WNDCLASSA cls={0};cls.lpfnWndProc=window_proc;cls.hInstance=instance;cls.lpszClassName="OnlineStartupTest";assert(RegisterClassA(&cls));HWND w=CreateWindowA(cls.lpszClassName,"Online startup fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,640,480,NULL,NULL,instance,NULL);assert(w);
 SetTimer(w,19,25,accept_dialog);SendMessageA(w,WM_COMMAND,102,0);assert(connection&&connection->client.login_valid&&!session);puts("PASS production Connect dialog/auth/cached level");
 selection.ship_index=0;SetTimer(w,20,25,accept_dialog);SendMessageA(w,WM_COMMAND,40008,0);assert(session&&session->online&&session->start.local_id==42&&!session->ships.players[0]&&session->ships.players[7]);
 SetForegroundWindow(w);SetFocus(w);for(int retry=0;retry<20;retry++){drain();Sleep(10);}session->host.key_state=held;session->host.key_state_event=held;
 SniperShipObject*local=sniper_session_local(session);local->word[0x48/4]=2000000;local->word[0x4c/4]=2000000;int32_t x=local->word[0x48/4],y=local->word[0x4c/4];int fired=0;
 for(int i=0;i<120;i++){drain();Sleep(10);game_frame(w);assert(session);fired|=session->combat.count>0;}
 assert(fired&&(x!=local->word[0x48/4]||y!=local->word[0x4c/4]));assert(session->ships.player_count==2&&painter.order.local_id==42);puts("PASS localID42 live rendered controls/projectiles and remote registry");
 int file_notice=0;for(int i=0;i<16;i++){if(strstr(session->chat_lines[i],"WIRETEST.TXT"))file_notice=1;assert(!strstr(session->chat_lines[i],"wirebad.txt"));}assert(file_notice);puts("PASS file success notice and rejected-write notice suppression");
 assert(audio.device);voice_shift=1;
 for(int i=0;i<session->ships.player_count;i++)if(session->ordered[i]->word[0x44/4]==7)session->binding.selected=i;
 for(int slot=0;slot<2;slot++)for(int repeat=0;repeat<2;repeat++){
    memset(&session->ships.attachment_hud,0,sizeof session->ships.attachment_hud);
    SendMessageA(w,WM_KEYDOWN,VK_F11+slot,1);game_frame(w);
    assert(!session->lifecycle.key&&!session->lifecycle.repeat&&connection->client.cached[slot]);
    assert(received_voice.count==1&&received_voice.buffer[0]);
    for(int i=0;i<30;i++){drain();Sleep(10);game_frame(w);assert(session);}
    assert(received_voice.count==1&&received_voice.buffer[0]);
    int remote_notice=0;for(int i=0;i<4;i++)if(strstr(session->ships.attachment_hud.text[i],"Voice: Remote07,"))remote_notice=1;
    assert(remote_notice);
 }
 voice_shift=0;puts("PASS ShiftF11/F12 first/repeat wire sends,local and remote dynamic voice playback");
 /* Exercise production inactive branch directly: synthetic OS activation
  * messages sent through Wine DirectDraw subclass corrupt cooperative mode. */
 window_proc(w,WM_ACTIVATEAPP,FALSE,0);uint32_t before=session->frame.clock.tick;for(int i=0;i<30;i++){Sleep(10);game_frame(w);}puts("TRACE inactive simulation done");assert(session&&session->frame.clock.tick>before);window_proc(w,WM_ACTIVATEAPP,TRUE,0);SetForegroundWindow(w);SetFocus(w);for(int retry=0;retry<20;retry++){drain();Sleep(10);}puts("TRACE production active handler restored");
 SendMessageA(w,WM_KEYDOWN,VK_F12,1);game_frame(w);assert(local->word[0x5c/4]<0);stop_game(w);assert(!session&&connection&&!game_display.draw);for(int i=0;i<30;i++){sniper_network_client_poll(&connection->client);Sleep(10);}SendMessageA(w,WM_COMMAND,103,0);assert(!connection);DestroyWindow(w);puts("PASS inactive network simulation,wire death,game leave,disconnect and display cleanup");return 0;
}
