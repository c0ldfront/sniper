#include "offline_session.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct{uint32_t now;int exits;} Host;
static uint32_t now(void *p){return ((Host *)p)->now;}
static int32_t key(void *p,int32_t vk){(void)p;(void)vk;return 0;}
static void post(void *p){((Host *)p)->exits++;}
static void action(SniperOfflineSession *s,Host *h,int k,int c){h->now+=10;s->lifecycle.key=k;s->lifecycle.character=c;assert(sniper_session_step(s));assert(!s->lifecycle.key&&!s->lifecycle.repeat&&!s->lifecycle.character);}
int main(void){
 Host h={1000,0};SniperSessionHost host={.clock_ms=now,.key_state=key,.post_command=post,.user=&h};SniperTileSize sizes[256]={{0}};
 for(int i=216;i<220;++i)sizes[i]=(SniperTileSize){32,32};
 SniperOfflineSession *s=sniper_session_create_editor(&host,NULL,0,sizes,640,480,87,100,100,1);assert(s);
 assert(s->frame.special&&!s->overlay_mode&&!s->radar_enabled&&!s->chat.enabled);
 action(s,&h,32,0);assert(s->map[100*1024+100]==1&&s->editor_records.dirty&&s->editor_records.count==1);
 assert(s->frame.camera.x==100*16-320+8&&s->frame.camera.y==100*16-320+8);
 action(s,&h,0,'s');assert(s->editor.selection);
 action(s,&h,39,0);action(s,&h,40,0);action(s,&h,0,'s');
 assert(!s->editor.selection&&s->editor.clipboard&&s->editor.clipboard_width==2&&s->editor.clipboard_height==2);
 assert(s->editor.clipboard[0]==1&&s->editor.clipboard[1]==0&&s->editor.clipboard[2]==0&&s->editor.clipboard[3]==0);
 s->editor.x=200;s->editor.y=200;action(s,&h,0,'p');assert(s->map[200*1024+200]==1&&s->editor_records.count==2);
 action(s,&h,0,'e');assert(s->map[200*1024+200]==0&&s->editor_records.count==1);
 action(s,&h,0,'s');assert(!s->editor.clipboard);
 action(s,&h,0,'A');assert(s->map[200*1024+200]==216&&s->map[201*1024+201]==250);
 assert(s->editor_records.count==2);action(s,&h,46,0);assert(!s->map[200*1024+200]&&!s->map[201*1024+201]&&s->editor_records.count==1);
 action(s,&h,115,0);assert(s->radar_enabled&&s->radar_dirty);action(s,&h,13,0);assert(!s->radar_enabled);
 action(s,&h,27,0);assert(h.exits==1&&!s->frame.auto_pending);
 assert(s->combat.count==0);assert(s->editor_records.checksum==0);
 size_t n=sniper_editor_records_bytes(&s->editor_records,NULL,0);uint8_t out[4];assert(n==4);sniper_editor_records_bytes(&s->editor_records,out,4);
 SniperEditorRecords reload={0};assert(sniper_editor_records_load(&reload,out,4));assert(reload.count==1&&!reload.dirty);sniper_editor_records_free(&reload);
 sniper_session_destroy(s);
 puts("PASS editor session: painting,selection,clipboard,paste/erase,large footprints,records,camera,radar,Escape and cleanup");return 0;
}
