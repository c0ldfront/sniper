/* Includes the production window procedure so menu/dialog/lifecycle wiring is
 * exercised, not a second implementation of the app adapter. */
#include "../platform/startup.c"
static VOID CALLBACK accept_ship(HWND w,UINT message,UINT_PTR timer,DWORD time) {
    (void)message;(void)time;
    HWND dialog=GetLastActivePopup(w);
    if(dialog!=w){KillTimer(w,timer);PostMessageA(dialog,WM_COMMAND,IDOK,0);}
}
static VOID CALLBACK cancel_ship(HWND w,UINT message,UINT_PTR timer,DWORD time) {
    (void)message;(void)time;
    HWND dialog=GetLastActivePopup(w);
    if(dialog!=w){KillTimer(w,timer);PostMessageA(dialog,WM_COMMAND,IDCANCEL,0);}
}
static void drain(void) {
    MSG message;
    while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)) {
        TranslateMessage(&message);DispatchMessageA(&message);
    }
}
static int mouse_action(HWND w,int cycle,const char *phase) {
    (void)w;char ready[64],ack[64];
    snprintf(ready,sizeof ready,"%s-ready-%d",phase,cycle);snprintf(ack,sizeof ack,"%s-sent-%d",phase,cycle);
    FILE *f=fopen(ready,"w");if(!f)return 0;fclose(f);
    for(int retry=0;retry<200;++retry){drain();Sleep(10);f=fopen(ack,"r");if(f){fclose(f);drain();return 1;}}
    return 0;
}
static HRESULT (*original_extension)(void *,SniperSceneStage);
static unsigned inactive_draw_calls;
static HRESULT count_inactive_draw(void *user,SniperSceneStage stage){
    (void)user;(void)stage;++inactive_draw_calls;return S_OK;
}
static HRESULT trace_extension(void *user,SniperSceneStage stage){
    printf("spectator extension %d begin\n",stage);HRESULT hr=original_extension(user,stage);
    printf("spectator extension %d hr=%08lx\n",stage,(unsigned long)hr);return hr;
}
static uint32_t trace_present(void *user){
    SniperSessionPainter *p=user;HRESULT hr=IDirectDrawSurface_Flip(p->display->primary,NULL,DDFLIP_WAIT);
    printf("spectator Flip hr=%08lx foreground=%d active=%d\n",(unsigned long)hr,GetForegroundWindow()==p->display->window,application_active);
    if(FAILED(hr)){p->scene.result=hr;return 0;}return (uint32_t)hr;
}
int main(void) {
    sniper_numeric_initialize();
    setvbuf(stdout,NULL,_IONBF,0);
    instance=GetModuleHandleA(NULL);InitCommonControls();sniper_options_default(&options);
    WNDCLASSA cls={0};cls.lpfnWndProc=window_proc;cls.hInstance=instance;cls.lpszClassName="SniperAppTest";
    if(!RegisterClassA(&cls))return 1;
    HWND w=CreateWindowA(cls.lpszClassName,"Sniper app integration",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,640,480,NULL,NULL,instance,NULL);
    if(!w)return 2;
    for(int cycle=0;cycle<2;++cycle) {
        int32_t expected_ticker=ticker_index;
        uint32_t expected_chat_tick=chat_build_tick;
        selection.ship_index=(uint32_t)cycle;
        SetTimer(w,17,50,accept_ship);
        SendMessageA(w,WM_COMMAND,40008,0);
        if(!session)return 3;
        if(session->ticker_index!=expected_ticker||
           game_graphics.chat.last_build_tick!=expected_chat_tick||
           (cycle&&strcmp(session->chat.macros[0],"X")))return 30;
        /* Native Play opens the selection dialog even during a game. Cancel
         * must preserve the live session rather than acting as Stop. */
        SniperOfflineSession *running=session;
        uint32_t selected_ship=selection.ship_index;
        session->chat.macros[0][0]='X';session->chat.macros[0][1]=0;
        SetTimer(w,18,50,cancel_ship);
        SendMessageA(w,WM_COMMAND,40008,0);
        if(session!=running||selection.ship_index!=selected_ship||
           strcmp(session->chat.macros[0],"X")||!game_display.draw)return 28;
        puts("PASS active Play cancellation preserves session, selection and macro");
        /* Exercise production inactive stepping: simulation timers advance,
         * queued input survives, and no drawing stage is entered. */
        int32_t timer_before=session->ships.players[0]->word[0x15c/4];
        session->ships.players[0]->word[0x15c/4]=1000;
        session->lifecycle.key=VK_F12;
        HRESULT (*saved_extension)(void *,SniperSceneStage)=painter.scene.extension;
        painter.scene.extension=count_inactive_draw;inactive_draw_calls=0;
        /* Invoke our branch directly: synthetic SendMessage also reaches
         * Wine's DirectDraw subclass without a real OS focus transition. */
        window_proc(w,WM_ACTIVATEAPP,FALSE,0);
        Sleep(30);game_frame(w);
        if(!session||session->ships.players[0]->word[0x15c/4]>=1000||
           session->lifecycle.key!=VK_F12||inactive_draw_calls)return 29;
        session->ships.players[0]->word[0x15c/4]=timer_before;
        session->lifecycle.key=0;painter.scene.extension=saved_extension;
        window_proc(w,WM_ACTIVATEAPP,TRUE,0);
        puts("PASS inactive simulation advances, input remains queued, rendering skipped");
        int32_t x=session->ships.players[0]->word[0x48/4],y=session->ships.players[0]->word[0x4c/4];
        SetForegroundWindow(w);SetFocus(w);
        char ready[40],ack[40];snprintf(ready,sizeof ready,"input-ready-%d",cycle);snprintf(ack,sizeof ack,"input-sent-%d",cycle);
        FILE *marker=fopen(ready,"w");if(!marker)return 14;fclose(marker);
        int injected=0;
        for(int retry=0;retry<200;++retry){drain();Sleep(10);marker=fopen(ack,"r");if(marker){fclose(marker);injected=1;break;}}
        if(!injected)return 15;
        int shots=0;
        for(int i=0;i<600;++i) {
            drain();Sleep(10);game_frame(w);
            if(i==50)printf("held up=%lx ctrl=%lx focus=%d tick=%lu\n",(unsigned long)GetAsyncKeyState(VK_UP),(unsigned long)GetAsyncKeyState(VK_CONTROL),GetForegroundWindow()==w,(unsigned long)session->frame.clock.tick);
            if(!session)return 4;
            shots|=session->combat.count>0;
        }
        keybd_event(VK_UP,0,KEYEVENTF_KEYUP,0);keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0);
        printf("cycle%d active%d key%lx shots%d delta%ld,%ld energy%ld\n",cycle,application_active,(unsigned long)GetAsyncKeyState(VK_UP),shots,(long)(session->ships.players[0]->word[0x48/4]-x),(long)(session->ships.players[0]->word[0x4c/4]-y),(long)session->ships.players[0]->word[0x5c/4]);
        if(!shots || (x==session->ships.players[0]->word[0x48/4]&&y==session->ships.players[0]->word[0x4c/4]))return 5;
        SendMessageA(w,WM_KEYDOWN,VK_F2,1);game_frame(w);
        if(!session||session->overlay_mode!=2||!game_graphics.statistics.surface)return 19;
        if(!mouse_action(w,cycle,"mouse"))return 20;
        game_frame(w);if(!session->mouse.drag)return 21;
        if(!mouse_action(w,cycle,"drag"))return 22;
        game_frame(w);if(session->start.history_size!=10)return 23;
        if(!mouse_action(w,cycle,"release"))return 24;
        game_frame(w);if(session->mouse.drag)return 25;
        SendMessageA(w,WM_KEYDOWN,VK_ESCAPE,1);game_frame(w);
        if(!session||session->overlay_mode!=0||session->frame.auto_pending)return 26;
        puts("PASS live mouse border drag, history resize, release and Escape overlay close");
        SendMessageA(w,WM_KEYDOWN,VK_F12,1);game_frame(w);
        if(session->ships.players[0]->word[0x5c/4]>=0)return 6;
        /* Seed exact energy boundaries to exercise deferred shutdown without
         * waiting for a second full respawn/recharge cycle. */
        session->ships.players[0]->word[0x5c/4]=500000;
        SendMessageA(w,WM_KEYDOWN,VK_ESCAPE,1);game_frame(w);
        if(!session || !session->frame.auto_pending)return 16;
        drain();if(!session)return 17;
        session->ships.players[0]->word[0x5c/4]=500000;
        game_frame(w);drain();if(!session)return 18;
        session->ships.players[0]->word[0x5c/4]=500001;
        game_frame(w);drain();
        if(session || game_display.draw || game_graphics.palette || GetCapture()==w||statistics_rows!=10)return 7;
        printf("PASS cycle%d: Play Game dialog, live keyboard movement/fire, F12, energy-gated Escape, queued shutdown, cleanup\n",cycle);
    }
    puts("START spectator startup");
    selection.ship_index=4;
    SetTimer(w,17,50,accept_ship);
    SendMessageA(w,WM_COMMAND,40008,0);
    if(!session)return 8;
    puts("START spectator frame");
    SetForegroundWindow(w);SetFocus(w);puts("START spectator drain");drain();
    puts("START spectator present");
    original_extension=painter.scene.extension;painter.scene.extension=trace_extension;painter.scene.present=trace_present;
    HRESULT spectator_hr=sniper_session_present(&painter);
    printf("spectator result=%08lx\n",(unsigned long)spectator_hr);
    if(FAILED(spectator_hr)||!session||session->ships.players[0]->word[0x40/4]!=4)return 27;
    puts("PASS spectator frame");
    stop_game(w);
    selection.ship_index=0;
    if(rename("LOCAL.LVL","LOCAL.test-lvl"))return 9;
    HRESULT missing=start_game(w);
    if(rename("LOCAL.test-lvl","LOCAL.LVL"))return 10;
    if(missing!=HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || session || game_display.draw)return 11;
    if(FAILED(start_game(w)))return 12;
    DestroyWindow(w);
    if(session || game_display.draw || game_graphics.palette)return 13;
    puts("PASS spectator startup, missing level, active-window destruction cleanup");return 0;
}
