#include "editor_render.h"
static int32_t add(int32_t a,int32_t b){return (int32_t)((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return (int32_t)((uint32_t)a-(uint32_t)b);}
static int32_t tile(int32_t a){return (int32_t)((uint32_t)a<<4);}
static int emit(SniperEditorDraw f,void *u,int k,int32_t x,int32_t y,int32_t n,int32_t r,int32_t b,int32_t c){
 SniperEditorDrawCommand cmd={k,x,y,n,r,b,c};return f(u,&cmd);
}
void sniper_editor_render(const SniperEditorRenderState *s,SniperEditorDraw f,void *u){
 int32_t l=0,t=0,r=0,b=0,color=-1,x=sub(tile(s->cursor_x),s->camera_x),y=sub(tile(s->cursor_y),s->camera_y);
 emit(f,u,SNIPER_EDITOR_CURSOR,x,y,0,0,0,0);
 if(s->selection_active){
  int32_t minx=s->x1<s->x2?s->x1:s->x2,maxx=s->x1>s->x2?s->x1:s->x2;
  int32_t miny=s->y1<s->y2?s->y1:s->y2,maxy=s->y1>s->y2?s->y1:s->y2;
  l=sub(tile(minx),s->camera_x);t=sub(tile(miny),s->camera_y);
  r=add(l,tile(add(sub(maxx,minx),1)));b=add(t,tile(add(sub(maxy,miny),1)));color=s->selection_color;
 }else if(s->clipboard_active){l=x;t=y;r=add(l,tile(s->clipboard_width));b=add(t,tile(s->clipboard_height));color=s->clipboard_color;}
 if(color!=-1){
  if(l<0)l=0;
  if(t<0)t=0;
  if(r>=s->screen_width)r=sub(s->screen_width,1);
  if(b>=s->screen_height)b=sub(s->screen_height,1);
  if(emit(f,u,SNIPER_EDITOR_LOCK,0,0,0,0,0,0)){
   int32_t n=add(sub(r,l),1),h=sub(b,t);
   if(t!=0)emit(f,u,SNIPER_EDITOR_HORIZONTAL,l,t,n,0,0,color);
   if(sub(b,s->screen_height)!=-1)emit(f,u,SNIPER_EDITOR_HORIZONTAL,l,b,n,0,0,color);
   if(l!=0)emit(f,u,SNIPER_EDITOR_VERTICAL,l,t,h,0,0,color);
   if(sub(r,s->screen_width)!=-1)emit(f,u,SNIPER_EDITOR_VERTICAL,r,t,h,0,0,color);
   emit(f,u,SNIPER_EDITOR_UNLOCK,0,0,0,0,0,0);
  }
 }
 x=sub(s->screen_width,304)/2;
 emit(f,u,SNIPER_EDITOR_RECTANGLE,sub(x,1),7,0,add(x,304),136,s->palette_background_color);
 emit(f,u,SNIPER_EDITOR_PALETTE,x,8,0,0,0,0);
 {int32_t n=sub(s->selected_tile,1);emit(f,u,SNIPER_EDITOR_CURSOR,add(x,tile(n%19)),add(8,tile(n/19)),0,0,0,0);}
}
