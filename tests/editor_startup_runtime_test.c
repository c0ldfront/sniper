/* Own isolated runtime working directory: this test creates/replaces LOCAL.LVL. */
#include "../platform/startup.c"
static int check_file(const uint8_t *want,size_t n){
 FILE *f=fopen("LOCAL.LVL","rb");if(!f)return 0;uint8_t data[64];size_t got=fread(data,1,sizeof data,f);fclose(f);return got==n&&!memcmp(data,want,n);
}
static void packed(uint8_t *b,int x,int y,int tile){uint32_t r=(uint32_t)x|((uint32_t)y<<12)|((uint32_t)tile<<24);for(int i=0;i<4;++i)b[i]=(uint8_t)(r>>(8*i));}
int main(void){
 instance=GetModuleHandleA(NULL);sniper_options_default(&options);options.value[SNIPER_SOUND]=0;
 uint8_t records[8];packed(records,1,1,20);FILE *f=fopen("LOCAL.LVL","wb");if(!f)return 1;fwrite(records,1,4,f);fclose(f);
 HWND w=CreateWindowA("STATIC","editor startup",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,640,480,NULL,NULL,instance,NULL);if(!w)return 2;
 window_proc(w,WM_COMMAND,40009,0);if(!session||!session->frame.special)return 3;
 session->editor.x=30;session->editor.y=40;session->editor.tile=19;
 window_proc(w,WM_KEYDOWN,VK_SPACE,0);game_frame(w);if(!session||session->map[40*1024+30]!=19)return 4;
 window_proc(w,WM_USER+1,0,0);if(session)return 5;
 packed(records+4,30,40,19);if(!check_file(records,8))return 6;
 window_proc(w,WM_COMMAND,40009,0);if(!session||session->editor.x!=30||session->editor.y!=40||session->editor.tile!=19||session->editor_records.dirty)return 7;
 if(session->map[40*1024+30]!=19)return 8;
 game_frame(w);if(!session)return 9;
 /* Replacement failure must preserve old level and retain serialized recovery. */
 if(!sniper_editor_records_place(&session->editor_records,30,40,20))return 10;
 HANDLE held=CreateFileA("LOCAL.LVL",GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(held==INVALID_HANDLE_VALUE)return 11;
 char recovery[MAX_PATH];HRESULT hr=save_editor_level(&session->editor_records,recovery);CloseHandle(held);
 if(SUCCEEDED(hr)||!recovery[0]||!check_file(records,8))return 12;
 DeleteFileA(recovery);session->editor_records.dirty=0;
 window_proc(w,WM_KEYDOWN,VK_ESCAPE,0);game_frame(w);if(!session)return 13;
 /* Native post is queued; consume only the requested exit. */
 MSG msg;int exit=0;while(PeekMessageA(&msg,w,WM_USER+1,WM_USER+1,PM_REMOVE)){window_proc(w,msg.message,msg.wParam,msg.lParam);exit=1;}
 if(!exit||session)return 14;
 if(!check_file(records,8))return 15;
 sniper_audio_close(&audio);DestroyWindow(w);
 puts("PASS editor startup40009: real paint,atomic save,reload,cursor lifetime,replacement failure retention,Escape cleanup");return 0;
}
