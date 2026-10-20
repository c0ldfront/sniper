/* Reconstructed startup and partial offline game integration.
 * Entry: 40ce80; window: 40ae10; license: 410080; about: 40f5f0.
 * Original payloads supply UI art/templates; no original code is executed. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include "settings.h"
#include "ship_dialog.h"
#include "display.h"
#include "session_painter.h"
#include "audio_player.h"
#include "audio_message.h"
#include "macro_dialog.h"
#include "online_connection.h"
#include "network_commands.h"
#include "network_file_receive.h"
#include "idle_bitmap.h"
#include "startup_random.h"
#include <time.h>
#include "../src/numeric_environment.h"
#include "../src/online_session.h"
#include "../src/network_checksum.h"
#include "../src/network_lifecycle.h"
#include "../src/network_compression.h"
#include <mmsystem.h>
static HINSTANCE instance;
static SniperOptions options;
static int settings_ready;
static Sniper87ShipDialogState selection={0,4};
static int32_t statistics_rows=20;
static char chat_macros[8][82];
static SniperDisplay display={640,480,4};
static SniperGameDisplay game_display;
static SniperGameGraphics game_graphics;
static SniperOfflineSession *session;
static SniperSessionPainter painter;
static int application_active=1;
static SniperFrameClock process_clock;
static Sniper87StartupRandom startup_random={1,0};
static Sniper87ConnectionSettings connection_settings;
static Sniper87OnlineConnection *connection;
static SniperNetworkFileReceive file_receive={.file=INVALID_HANDLE_VALUE};
static int suppress_file_notice;
static int32_t ticker_index;
static uint32_t chat_build_tick;
static SniperAudioVoice received_voice;
static int32_t editor_x,editor_y,editor_tile=1;
static SniperAudioPlayer audio;
static int audio_initialized;
static void connection_event(void *user,const SniperGameEvent *e){
    (void)user;
    if(e->kind==SNIPER_NET_FILE_OPEN)suppress_file_notice=0;
    if(sniper_network_file_receive_event(&file_receive,e)<0)suppress_file_notice=1;
}
static int connection_accept_event(void *user,const SniperGameEvent *e){
    (void)user;
    if(e->kind==SNIPER_NET_CHAT&&suppress_file_notice&&e->data&&e->size>=15&&!memcmp(e->data,"File received: ",15)){
        suppress_file_notice=0;return 0;
    }
    return 1;
}
static void disconnect_connection(void){
    sniper_network_file_receive_clear(&file_receive);suppress_file_notice=0;
    sniper87_online_disconnect(connection);connection=NULL;
}
static void game_audio(void *user,int32_t resource,const SniperAudioRequest *request){(void)user;sniper_audio_play(&audio,resource,request);}
static void game_audio_stop(void *user,int32_t resource){(void)user;sniper_audio_stop(&audio,resource);}
static uint32_t game_clock(void *user) {(void)user;return GetTickCount();}
static int32_t game_key(void *user,int32_t key) {(void)user;return GetAsyncKeyState(key);}
static void game_voice(void *user,int32_t sender,const uint8_t *data,size_t length){
    SniperOfflineSession *s=user;if(!s||!data||length>INT32_MAX)return;
    SniperShipObject *p=sender>=0&&sender<255?s->ships.players[sender]:NULL;
    if(p){char text[80];snprintf(text,sizeof text,"Voice: %.31s,%.31s",(const char*)p,(const char*)p+32);sniper_notice_push(&s->ships.attachment_hud,text,300,3);}
    (void)sniper_audio_message(&audio,&received_voice,data,(int32_t)length);
}
static int32_t game_key_event(void *user,int32_t key) {(void)user;return GetKeyState(key);}
static uint32_t game_joystick(void *user,uint32_t id,uint32_t info[13]) {
    (void)user;
    JOYINFOEX joy;memcpy(&joy,info,sizeof joy);
    MMRESULT result=joyGetPosEx(id,&joy);memcpy(info,&joy,sizeof joy);return result;
}
static void game_post(void *user){PostMessageA((HWND)user,WM_USER+1,0,0);}
static void game_cursor(void *user,int32_t x,int32_t y) {(void)user;SetCursorPos(x,y);}
/* Safety adaptation: preserve current working level until a complete sibling
 * replacement has been flushed. Native40a370 deletes before opening wb. */
static HRESULT save_editor_level(const SniperEditorRecords *records,char recovery[MAX_PATH]){
    recovery[0]=0;if(!sniper_editor_records_save_needed(records))return S_FALSE;
    size_t bytes=sniper_editor_records_bytes(records,NULL,0);
    uint8_t *data=malloc(bytes?bytes:1);if(!data)return E_OUTOFMEMORY;
    sniper_editor_records_bytes(records,data,bytes);
    if(!GetTempFileNameA(".","s87",0,recovery)){free(data);return HRESULT_FROM_WIN32(GetLastError());}
    FILE *f=fopen(recovery,"wb");if(!f){free(data);return E_FAIL;}
    int failed=fwrite(data,1,bytes,f)!=bytes;
    if(fflush(f)!=0)failed=1;
    if(_commit(_fileno(f))!=0)failed=1;
    if(fclose(f)!=0)failed=1;
    free(data);if(failed)return E_FAIL;
    if(!MoveFileExA(recovery,"LOCAL.LVL",MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return HRESULT_FROM_WIN32(GetLastError());
    recovery[0]=0;return S_OK;
}
static void stop_game(HWND w) {
    SniperOfflineSession *old=session;session=NULL;
    if(old&&old->frame.local)startup_random.frequency=old->frame.frequency;
    HRESULT save_hr=S_FALSE;char recovery[MAX_PATH]={0};
    if(old&&old->frame.special){editor_x=old->editor.x;editor_y=old->editor.y;editor_tile=old->editor.tile;
        save_hr=save_editor_level(&old->editor_records,recovery);}
    if(old){ticker_index=old->ticker_index;chat_build_tick=game_graphics.chat.last_build_tick;statistics_rows=old->start.history_size;selection.history_size=old->chat_rows;memcpy(chat_macros,old->chat.macros,sizeof chat_macros);}
    ReleaseCapture();
    sniper_audio_stop_all(&audio);
    sniper_audio_voice_close(&received_voice);
    sniper_audio_close(&audio);audio_initialized=0;
    sniper_session_destroy(old);
    sniper_game_graphics_close(&game_graphics);
    sniper_game_display_close(&game_display);
    InvalidateRect(w,NULL,TRUE);
    if(FAILED(save_hr)){
        char message[512];snprintf(message,sizeof message,"Could not save LOCAL.LVL (error 0x%08lx). The previous level was preserved.%s%s",
            (unsigned long)save_hr,recovery[0]?" Temporary recovery file: ":"",recovery);
        MessageBoxA(w,message,"Sniper editor save failed",MB_OK|MB_ICONERROR);
    }
}
static HRESULT start_game_mode(HWND w,int editor) {
    if(session)return S_FALSE;
    if(!editor&&selection.ship_index>4)return E_INVALIDARG;
    FILE *f=(!editor&&connection)?NULL:fopen("LOCAL.LVL","rb");long size=(!editor&&connection)?(long)connection->level_size:0;
    if(!f&&!editor&&!connection)return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    if(f){if(fseek(f,0,SEEK_END)!=0){fclose(f);return E_FAIL;}size=ftell(f);
        if(size<0||fseek(f,0,SEEK_SET)!=0){fclose(f);return E_FAIL;}}
    uint8_t *data=malloc(size?(size_t)size:1);
    if(!data){if(f)fclose(f);return E_OUTOFMEMORY;}
    if(!editor&&connection)memcpy(data,connection->level_bytes,(size_t)size);
    if(f){size_t got=fread(data,1,(size_t)size,f);int failed=ferror(f);fclose(f);
        if(failed||got!=(size_t)size){free(data);return E_FAIL;}}
    if(options.value[SNIPER_SOUND]&&!audio_initialized){sniper_audio_open(&audio,w,instance);sniper_audio_set_active(&audio,application_active);audio_initialized=1;}
    HRESULT hr=sniper_game_display_open(&game_display,w,display.width,display.height);
    if(SUCCEEDED(hr))hr=sniper_game_graphics_open(&game_graphics,&game_display,instance);
    if(SUCCEEDED(hr)) {
        game_graphics.chat.last_build_tick=chat_build_tick;
        SniperTileSize sizes[256]={{0}};
        for(int i=0;i<4;++i)sizes[216+i]=(SniperTileSize){game_graphics.map.special[i].descriptor.width,game_graphics.map.special[i].descriptor.height};
        char name[32];GetPrivateProfileStringA("Dialog","PlayerName","",name,sizeof name,"sniper.ini");
        SniperSessionHost host={.clock_ms=game_clock,.key_state=game_key,.joystick=game_joystick,.cursor=game_cursor,.post_command=game_post,.user=w,.key_state_event=game_key_event,.audio=game_audio,.audio_stop=game_audio_stop,.process_clock=&process_clock,.process_random_state=&startup_random.crt_state};
        if(editor)session=sniper_session_create_editor(&host,data,(size_t)size,sizes,display.width,display.height,startup_random.crt_state,editor_x,editor_y,editor_tile);
        else if(connection){
            session=sniper_online_session_begin(&host,&connection->client,data,(size_t)size,sizes,(int32_t)selection.ship_index,startup_random.frequency,options.value[SNIPER_RECEIVE_WAVE]&&options.value[SNIPER_SOUND],display.width,display.height,startup_random.crt_state);
            if(session)session->online->accept_event=connection_accept_event;
            while(session&&!session->online->ready){
                if(sniper_online_session_poll(session)<0||!sniper_network_transport_alive(&connection->client.transport,1)||(GetAsyncKeyState(VK_ESCAPE)&0x8000)){sniper_session_destroy(session);session=NULL;break;}
                Sleep(1);
            }
        }else session=sniper_session_create(&host,data,(size_t)size,sizes,name,(int32_t)selection.ship_index,startup_random.frequency,display.width,display.height,startup_random.crt_state);
        if(!session)hr=E_OUTOFMEMORY;
        else {
            session->voice_receive=game_voice;session->voice_user=session;
            session->ticker_index=ticker_index;session->active=application_active;
            session->control.engine_sound=options.value[SNIPER_ENGINE_SOUND];
            session->start.history_size=statistics_rows;
            session->chat_rows=selection.history_size<16?selection.history_size:16;
            memcpy(session->chat.macros,chat_macros,sizeof chat_macros);
            if(session->start.history_size>display.height/24)session->start.history_size=display.height/24;
            sniper_session_painter_init(&painter,session,&game_display,&game_graphics);
            SetCapture(w);
        }
    }
    free(data);
    if(FAILED(hr))stop_game(w);
    return hr;
}
static HRESULT start_game(HWND w){return start_game_mode(w,0);}
static void game_frame(HWND w) {
    if(!session)return;
    session->active=application_active;
    session->lifecycle.joystick_enabled=options.value[SNIPER_JOYSTICK];
    session->radar_small=options.value[SNIPER_SMALL_MAP];
    session->ticker_enabled=options.value[SNIPER_TICKER];
    session->stars_enabled=options.value[SNIPER_STARS];
    painter.hud.show_energy_bar=options.value[SNIPER_ENERGY_BAR];
    painter.options.low_detail=options.value[SNIPER_LOW_DETAIL];
    HRESULT hr=application_active?sniper_session_present(&painter):(sniper_session_step(session)?S_OK:E_FAIL);
    options.value[SNIPER_ENGINE_SOUND]=session->control.engine_sound;
    options.value[SNIPER_STARS]=session->stars_enabled;
    options.value[SNIPER_TICKER]=session->ticker_enabled;
    /* Native40d045 ignores failed frame return values and retries on the next
     * loop iteration. A lost display must not destroy the live session. */
    if(hr==DDERR_SURFACELOST)return;
    if(FAILED(hr)) {
        stop_game(w);
        char error[128];snprintf(error,sizeof error,"Gameplay stopped (error 0x%08lx).",(unsigned long)hr);
        MessageBoxA(w,error,"Sniper",MB_OK|MB_ICONERROR);
    }
}
static INT_PTR CALLBACK about_proc(HWND w, UINT m, WPARAM a, LPARAM b) {
    (void)b;
    if (m == WM_CLOSE) { EndDialog(w, (INT_PTR)a); return TRUE; }
    if (m == WM_INITDIALOG) return TRUE;
    if (m == WM_COMMAND) {
        if (LOWORD(a) == IDOK || LOWORD(a) == IDCANCEL)
            PostMessageA(w, WM_CLOSE, 0, 0);
        return TRUE;
    }
    return FALSE;
}
static INT_PTR CALLBACK license_proc(HWND w, UINT m, WPARAM a, LPARAM b) {
    (void)b;
    switch (m) {
    case WM_INITDIALOG: {
        FILE *f = fopen("LICENSE.TXT", "r");
        SetWindowLongPtrA(w, DWLP_USER, 0);
        if (f) {
            char *text = malloc(0x8000);
            if (text) {
                size_t n = fread(text, 1, 0x7f00, f);
                text[n] = 0;
                SetDlgItemTextA(w, 1018, text);
                free(text);
            }
            fclose(f);
        }
        SendDlgItemMessageA(w, 1018, EM_SETSEL, 0, 0);
        return TRUE;
    }
    case WM_CLOSE:
        EndDialog(w, GetWindowLongPtrA(w, DWLP_USER)); return TRUE;
    case WM_COMMAND:
        switch (LOWORD(a)) {
        case IDOK: SetWindowLongPtrA(w, DWLP_USER, 1); /* fall through */
        case IDCANCEL: PostMessageA(w, WM_CLOSE, 0, 0); break;
        case 1018: SendDlgItemMessageA(w, 1018, EM_SETSEL, 0, 0); break;
        }
        return TRUE;
    }
    return FALSE;
}
static void paint_idle(HWND w) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(w, &ps);
    sniper87_idle_bitmap_paint(w,dc,instance);
    EndPaint(w, &ps);
}
static LRESULT CALLBACK window_proc(HWND w, UINT m, WPARAM a, LPARAM b) {
    switch (m) {
    case WM_DESTROY:
        if(session)stop_game(w);
        disconnect_connection();
        sniper_audio_voice_close(&received_voice);sniper_audio_close(&audio);audio_initialized=0;
        if (settings_ready) {
            char number[32];
            sniper_options_save(&options);
            sniper87_startup_frequency_save(startup_random.frequency);
            sniper_display_save(&display);
            snprintf(number,sizeof number,"%ld",(long)(int32_t)selection.ship_index);
            WritePrivateProfileStringA("Dialog","PlayerShipType",number,"sniper.ini");
            WritePrivateProfileStringA("Dialog","PlayerName",connection_settings.player_name,"sniper.ini");
            WritePrivateProfileStringA("Dialog","PlayerPassword",connection_settings.password,"sniper.ini");
            WritePrivateProfileStringA("Dialog","ServerArena",connection_settings.arena,"sniper.ini");
            snprintf(number,sizeof number,"%ld",(long)selection.history_size);
            WritePrivateProfileStringA("Misc","MessageHistorySize",number,"sniper.ini");
            snprintf(number,sizeof number,"%ld",(long)statistics_rows);
            WritePrivateProfileStringA("Misc","MaxStatLength",number,"sniper.ini");
            for(int i=0;i<8;++i){char key[16];snprintf(key,sizeof key,"ShiftF%d",i+1);
                WritePrivateProfileStringA("Message",key,chat_macros[i],"sniper.ini");}
        }
        PostQuitMessage(0); return 0;
    case WM_INITMENU: {
        if(connection&&!sniper_network_transport_alive(&connection->client.transport,1)){
            if(session)stop_game(w);
            disconnect_connection();
        }
        HMENU menu=GetMenu(w);
        sniper_options_menu(menu,&options);
        sniper_display_menu(menu,&display);
        EnableMenuItem(menu,102,MF_BYCOMMAND|(connection?MF_GRAYED:MF_ENABLED));
        const unsigned unavailable[]={103,40012,40014,40030,40031};
        for(unsigned i=0;i<sizeof unavailable/sizeof unavailable[0];++i)
            EnableMenuItem(menu,unavailable[i],MF_BYCOMMAND|(connection?MF_ENABLED:MF_GRAYED));
        return 0;
    }
    case WM_ERASEBKGND:if(session)return TRUE;break;
    case WM_SIZE: InvalidateRect(w, NULL, TRUE); break;
    case WM_PAINT:
        if(session){PAINTSTRUCT ps;BeginPaint(w,&ps);EndPaint(w,&ps);}
        else paint_idle(w);
        return 0;
    case WM_ACTIVATEAPP:
        application_active=(int)a;
        sniper_audio_set_active(&audio,(int)a);
        if(a)sniper_frame_clock_rebase(&process_clock,GetTickCount());
        if(session) {
            session->active=(int)a;
            if(a){SetCapture(w);session->frame.clock.previous=process_clock.previous;}
            else ReleaseCapture();
        }
        break;
    case WM_USER+1:
        if(session){stop_game(w);SetCursor(LoadCursorA(NULL,IDC_ARROW));}
        return 0;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if(session) {
            POINT point;if(!GetCursorPos(&point))break;
            int64_t dx=(int64_t)session->mouse.x-point.x,dy=(int64_t)session->mouse.y-point.y;
            if(dx<0)dx=-dx;
            if(dy<0)dy=-dy;
            if(m!=WM_MOUSEMOVE||dx+dy>4) {
                session->mouse.x=point.x;session->mouse.y=point.y;
                session->start.cursor_tick=GetTickCount()/10u;
            }
            if(m!=WM_MOUSEMOVE) {
                session->lifecycle.mouse_x_event=m==WM_LBUTTONDOWN;
                session->lifecycle.mouse_y_event=m==WM_RBUTTONDOWN;
            }
        }
        break;
    case WM_KEYDOWN:
        if(session) {
            session->lifecycle.key=(int32_t)a;session->lifecycle.repeat=(int32_t)b;
        }
        break;
    case WM_SYSKEYDOWN:
        /* Native40b109 suppresses system-key/menu activation during play. */
        if(session)return 0;
        break;
    case WM_CHAR:
        if(session)session->lifecycle.character=(uint8_t)a;
        break;
    case WM_COMMAND:
        if (sniper_options_command(&options,LOWORD(a))) {
                    if(session)session->control.engine_sound=options.value[SNIPER_ENGINE_SOUND];
            if(!options.value[SNIPER_ENGINE_SOUND]){sniper_audio_stop(&audio,125);sniper_audio_stop(&audio,127);sniper_audio_stop(&audio,169);}
            return 0;
        }
        if (sniper_display_command(&display,LOWORD(a))) return 0;
        if(connection&&sniper87_network_command(w,instance,&connection->client,LOWORD(a)))return 0;
        switch (LOWORD(a)) {
        case 102: {
            INT_PTR choice=sniper87_connection_dialog(w,instance,&connection_settings);
            if(choice==1||choice==2){
                if(session)stop_game(w);
                disconnect_connection();
                char executable[MAX_PATH];GetModuleFileNameA(NULL,executable,sizeof executable);
                Sniper87OnlineOptions policy={".",sniper_network_checksum_file(executable),0};
                connection=sniper87_online_connect(w,instance,&connection_settings,&policy);
                if(connection){
                    if(!sniper_network_file_receive_init(&file_receive,connection_settings.host,connection_settings.port))disconnect_connection();
                    else{connection->client.host.event=connection_event;connection->client.host.event_user=&file_receive;}
                }
                if(connection&&choice==2){HRESULT hr=start_game(w);if(FAILED(hr))MessageBoxA(w,"Unable to enter game.","Sniper",MB_OK|MB_ICONERROR);}
            }return 0;
        }
        case 103:if(session)stop_game(w);disconnect_connection();return 0;
        case 40009: {
            if(session)stop_game(w);
            HRESULT hr=start_game_mode(w,1);
            if(FAILED(hr)){char message[128];snprintf(message,sizeof message,"Cannot start editor (error 0x%08lx).",(unsigned long)hr);MessageBoxA(w,message,"Sniper",MB_OK|MB_ICONERROR);}
            return 0;
        }
        case 40008:
            if((GetAsyncKeyState(VK_SHIFT)&0x8000)&&(GetAsyncKeyState(VK_CONTROL)&0x8000))return window_proc(w,WM_COMMAND,40009,0);
            if (sniper87_ship_dialog(w,instance,&selection)==1) {
                if(session)stop_game(w);
                WritePrivateProfileStringA("Dialog","PlayerShipPicked","1","sniper.ini");
                HRESULT hr=start_game(w);
                if(FAILED(hr)) {
                    char error[160];
                    snprintf(error,sizeof error,"Cannot start game (error 0x%08lx).",(unsigned long)hr);
                    MessageBoxA(w,error,"Sniper",MB_OK|MB_ICONERROR);
                }
            }
            return 0;
        case 40016: WinHelpA(w,"sniper.hlp",HELP_CONTENTS,0);return 0;
        case 40017:
            if(sniper87_macro_dialog(w,instance,session?session->chat.macros:chat_macros)==IDOK&&session)
                memcpy(chat_macros,session->chat.macros,sizeof chat_macros);
            return 0;
        case 40002: PostMessageA(w, WM_CLOSE, 0, 0); return 0;
        case 40003:
            DialogBoxParamA(instance, MAKEINTRESOURCEA(146), w, about_proc, 0);
            return 0;
        default:
            return DefWindowProcA(w,m,a,b);
        }
    }
    return DefWindowProcA(w, m, a, b);
}
int WINAPI WinMain(HINSTANCE inst, HINSTANCE previous, LPSTR command, int show) {
    sniper_numeric_initialize();
    WNDCLASSA cls = {0};
    HWND w;
    MSG msg;
    int result;
    (void)previous; (void)command; (void)show;
    instance = inst;
    sniper_options_default(&options);
    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = window_proc;
    cls.hInstance = inst;
    cls.hIcon = LoadIconA(inst, MAKEINTRESOURCEA(153));
    cls.hCursor = LoadCursorA(NULL, IDC_ARROW);
    cls.hbrBackground = GetStockObject(BLACK_BRUSH);
    cls.lpszMenuName = MAKEINTRESOURCEA(103);
    cls.lpszClassName = "SniperClass";
    if (!RegisterClassA(&cls)) return 1;
    w = CreateWindowExA(0, "SniperClass", "Sniper v.87", 0x10cf0000,
                        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
                        NULL, NULL, inst, NULL);
    if (!w) return 1;
    UpdateWindow(w);
    InitCommonControls();
    if (!GetPrivateProfileIntA("Misc", "Accepted", 0, "sniper.ini")) {
        if (DialogBoxParamA(inst, MAKEINTRESOURCEA(160), w, license_proc, 0) != 1) {
            DestroyWindow(w);
            return 0;
        }
        WritePrivateProfileStringA("Misc", "Accepted", "1", "sniper.ini");
    }
    sniper_options_load(&options);
    sniper87_startup_random_load(&startup_random,(uint32_t)time(NULL));
    selection.ship_index=GetPrivateProfileIntA("Dialog","PlayerShipType",0,"sniper.ini");
    selection.history_size=(int32_t)GetPrivateProfileIntA("Misc","MessageHistorySize",4,"sniper.ini");
    statistics_rows=(int32_t)GetPrivateProfileIntA("Misc","MaxStatLength",20,"sniper.ini");
    for(int i=0;i<8;++i){char key[16];snprintf(key,sizeof key,"ShiftF%d",i+1);
        GetPrivateProfileStringA("Message",key,"",chat_macros[i],82,"sniper.ini");}
    GetPrivateProfileStringA("Dialog","PlayerName","",connection_settings.player_name,sizeof connection_settings.player_name,"sniper.ini");
    GetPrivateProfileStringA("Dialog","PlayerPassword","",connection_settings.password,sizeof connection_settings.password,"sniper.ini");
    GetPrivateProfileStringA("Dialog","ServerArena","",connection_settings.arena,sizeof connection_settings.arena,"sniper.ini");
    settings_ready=1;
    sniper_display_load(w,&display);
    sniper_frame_clock_rebase(&process_clock,GetTickCount());
    /* Native40d426 queues the initial connection dialog after settings load. */
    PostMessageA(w,WM_COMMAND,102,0);
    for (;;) {
        if(!PeekMessageA(&msg,NULL,0,0,PM_NOREMOVE)) {
            if(session)game_frame(w);
            else if(connection){(void)sniper_network_client_poll(&connection->client);Sleep(1);}
            else WaitMessage();
            continue;
        }
        result=GetMessageA(&msg,NULL,0,0);
        if(result<=0)break;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        if(msg.message==WM_CHAR)game_frame(w);
    }
    return result < 0 ? 1 : (int)msg.wParam;
}
