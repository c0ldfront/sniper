/* Production startup WM_INITMENU,original MENU103,offline fixture state only. */
#include "../platform/startup.c"
int main(void){
    static const unsigned ids[]={102,103,40002,40008,40031,40014,40012,40030,40017,40004,40020,40005,40006,40007,40018,40019,40021,40023,40024,40029,40028,40026,40027,40025,40032,40016,40003};
    instance=GetModuleHandle(NULL);HMENU menu=LoadMenuA(instance,MAKEINTRESOURCEA(103));if(!menu)return 1;
    HWND w=CreateWindowA("STATIC","menu-test",WS_OVERLAPPEDWINDOW,0,0,640,480,NULL,menu,instance,NULL);if(!w)return 2;
    FILE *f=fopen("menu-init-oracle.txt","r");if(!f)return 3;int cases=0,failures=0,kind;
    while(fscanf(f,"%d %d %u %d",&display.width,&display.height,&display.supported,&kind)==4){
        for(int i=0;i<SNIPER_OPTION_COUNT;++i){int v;if(fscanf(f,"%d",&v)!=1)return 4;options.value[i]=v;}
        if(kind){connection=calloc(1,sizeof *connection);if(!connection)return 5;connection->socket.socket=(uintptr_t)-1;connection->download_file=INVALID_HANDLE_VALUE;connection->socket.started=kind==1;connection->socket.connected=kind==1;connection->client.transport.sender.connected=kind==1;}
        window_proc(w,WM_INITMENU,(WPARAM)menu,0);
        for(unsigned i=0;i<sizeof ids/sizeof *ids;++i){unsigned expected;if(fscanf(f,"%u",&expected)!=1)return 6;unsigned actual=GetMenuState(menu,ids[i],MF_BYCOMMAND)&(MF_CHECKED|MF_GRAYED);
            if(actual!=expected){if(failures<8)printf("mismatch case%d connection%d command%u expected%u got%u\n",cases,kind,ids[i],expected,actual);++failures;}}
        if(connection){connection->client.transport.sender.connected=0;connection->socket.started=0;sniper87_online_disconnect(connection);connection=NULL;}++cases;
    }
    fclose(f);
    session=(SniperOfflineSession *)(uintptr_t)1;
    if(window_proc(w,WM_SYSKEYDOWN,VK_MENU,0)!=0)++failures;
    session=NULL;
    for(unsigned i=0;i<SNIPER_OPTION_COUNT;++i){sniper_options_default(&options);int before=options.value[i];window_proc(w,WM_COMMAND,sniper_option_commands[i],0);if(options.value[i]!=!before)++failures;}
    const unsigned mode_ids[]={40004,40020,40005,40006,40007,40018,40019};
    const int widths[]={320,320,640,800,1024,1280,1600},heights[]={200,240,480,600,768,1024,1200};
    for(unsigned i=0;i<7;++i){window_proc(w,WM_COMMAND,mode_ids[i],0);if(display.width!=widths[i]||display.height!=heights[i])++failures;}
    const unsigned holes[]={40010,40011,40015,40022,65535};
    for(unsigned i=0;i<5;++i)if(window_proc(w,WM_COMMAND,holes[i],0)!=DefWindowProcA(w,WM_COMMAND,holes[i],0))++failures;
    DestroyWindow(w);printf("%s%d startup WM_INITMENU oracle cases,27 commands each,%d mismatches\n",failures?"FAIL":"PASS",cases,failures);return failures?1:0;
}
