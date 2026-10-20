/* Real external-process focus fixture. No synthetic activation messages. */
#include "../platform/startup.c"
static unsigned activated,deactivated;
static LRESULT CALLBACK observed(HWND w,UINT m,WPARAM a,LPARAM b){
    if(m==WM_ACTIVATEAPP){if(a)++activated;else ++deactivated;
        printf("OS activation=%lu foreground=%p focus=%p\n",(unsigned long)a,(void*)GetForegroundWindow(),(void*)GetFocus());}
    return window_proc(w,m,a,b);
}
static void pump(void){MSG m;while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageA(&m);}}
static void mark(const char *name){FILE *f=fopen(name,"w");if(f)fclose(f);}
static int wait_state(int active){for(unsigned n=0;n<1000;++n){pump();if(application_active==active)return 1;Sleep(10);}return 0;}
static IDirectDrawSurfaceVtbl traced_vtable;
static HRESULT (WINAPI *original_restore)(LPDIRECTDRAWSURFACE);
static HRESULT WINAPI trace_restore(LPDIRECTDRAWSURFACE surface){
    HRESULT hr=original_restore(surface);DDSURFACEDESC mode={0};mode.dwSize=sizeof mode;
    HRESULT mh=IDirectDraw_GetDisplayMode(game_display.draw,&mode);
    LPDIRECTDRAW4 d4=NULL;HRESULT coop=IDirectDraw_QueryInterface(game_display.draw,&IID_IDirectDraw4,(void**)&d4);if(SUCCEEDED(coop)){coop=IDirectDraw4_TestCooperativeLevel(d4);IDirectDraw4_Release(d4);}
    printf("primary Restore=%08lx cooperative=%08lx mode_hr=%08lx mode=%lux%lux%lu\n",(unsigned long)hr,(unsigned long)coop,(unsigned long)mh,(unsigned long)mode.dwWidth,(unsigned long)mode.dwHeight,(unsigned long)mode.ddpfPixelFormat.dwRGBBitCount);return hr;
}
static unsigned recoveries;static int (*saved_recover)(void *);
static int recover_trace(void *p){++recoveries;int ok=saved_recover(p);printf("recovery=%d primary=%08lx back=%08lx\n",ok,(unsigned long)IDirectDrawSurface_IsLost(game_display.primary),(unsigned long)IDirectDrawSurface_IsLost(game_display.backbuffer));return ok;}
int main(int argc,char **argv){
    (void)argv;setvbuf(stdout,NULL,_IONBF,0);instance=GetModuleHandleA(NULL);
    WNDCLASSA cls={0};cls.hInstance=instance;cls.lpszClassName=argc>1?"FocusExternal":"FocusSniper";cls.lpfnWndProc=argc>1?DefWindowProcA:observed;
    RegisterClassA(&cls);
    HWND w=CreateWindowA(cls.lpszClassName,cls.lpszClassName,WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,320,200,NULL,NULL,instance,NULL);
    if(!w)return 1;
    if(argc>1){mark("helper-ready");for(;;){pump();Sleep(10);}}
    InitCommonControls();sniper_options_default(&options);selection.ship_index=0;
    HRESULT hr=start_game(w);if(FAILED(hr)){printf("startup=%08lx\n",(unsigned long)hr);return 2;}
    SetForegroundWindow(w);SetFocus(w);pump();
    traced_vtable=*game_display.primary->lpVtbl;original_restore=traced_vtable.Restore;traced_vtable.Restore=trace_restore;game_display.primary->lpVtbl=&traced_vtable;
    saved_recover=painter.scene.recover;painter.scene.recover=recover_trace;
    for(unsigned n=0;n<60;++n){pump();hr=sniper_session_present(&painter);if(FAILED(hr))return 3;Sleep(10);}
    unsigned before=deactivated;mark("focus-away-ready");
    if(!wait_state(0)||deactivated==before){puts("FAIL no real deactivate");return 4;}
    DWORD pid=0;
    for(unsigned n=0;n<200;++n){pump();GetWindowThreadProcessId(GetForegroundWindow(),&pid);if(pid&&pid!=GetCurrentProcessId())break;Sleep(10);}
    printf("inactive foreground=%p external_pid=%lu self_pid=%lu\n",(void*)GetForegroundWindow(),(unsigned long)pid,(unsigned long)GetCurrentProcessId());
    if(pid==GetCurrentProcessId()||!pid)return 5;
    session->active=0;for(unsigned n=0;n<30;++n){pump();if(!sniper_session_step(session))return 6;Sleep(10);}
    before=activated;mark("focus-back-ready");
    if(!wait_state(1)||activated==before){puts("FAIL no real reactivate");return 7;}
    session->active=1;
    printf("restored foreground=%p game=%p focus=%p lost_primary=%08lx lost_back=%08lx\n",(void*)GetForegroundWindow(),(void*)w,(void*)GetFocus(),(unsigned long)IDirectDrawSurface_IsLost(game_display.primary),(unsigned long)IDirectDrawSurface_IsLost(game_display.backbuffer));
    for(unsigned n=0;n<120;++n){pump();hr=sniper_session_present(&painter);if(FAILED(hr)){printf("DISPLAY UNRECOVERED frame=%u hr=%08lx recoveries=%u\n",n,(unsigned long)hr,recoveries);
        if(hr!=DDERR_SURFACELOST){stop_game(w);return 8;}
        SniperOfflineSession *retained=session;unsigned prior_recoveries=recoveries;
        for(unsigned retry=0;retry<30;++retry){pump();game_frame(w);if(session!=retained||!game_display.draw)return 9;Sleep(10);}
        if(recoveries<=prior_recoveries)return 10;
        printf("PASS lost-frame lifecycle: session retained across30 production game_frame calls, retries=%u; display remains unrecovered\n",recoveries-prior_recoveries);
        stop_game(w);DestroyWindow(w);return 0;}Sleep(10);}
    printf("PASS external-process OS focus loss/regain, 120 resumed frames recoveries=%u\n",recoveries);
    stop_game(w);DestroyWindow(w);return 0;
}
