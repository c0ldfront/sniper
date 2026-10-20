#include "editor_painter.h"
#include "game_display.h"
#include "palette_resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int pixel(LPDIRECTDRAWSURFACE s,int x,int y,int value){
 DDSURFACEDESC d={0};d.dwSize=sizeof d;HRESULT hr=IDirectDrawSurface_Lock(s,NULL,&d,DDLOCK_WAIT,NULL);if(FAILED(hr))return 0;
 int got=((uint8_t *)d.lpSurface)[y*d.lPitch+x];IDirectDrawSurface_Unlock(s,d.lpSurface);
 if(got!=value)printf("pixel %d,%d got%d expected%d\n",x,y,got,value);
 return got==value;
}
int main(void){
 HINSTANCE inst=GetModuleHandleA(NULL);HWND w=CreateWindowA("STATIC","editor adapter",WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0,640,480,NULL,NULL,inst,NULL);
 SniperGameDisplay d={0};HRESULT hr=sniper_game_display_open(&d,w,640,480);if(FAILED(hr))return 1;
 LPDIRECTDRAWPALETTE palette=NULL;hr=sniper_palette_create_resource(d.draw,inst,107,&palette);if(FAILED(hr))return 2;
 IDirectDrawSurface_SetPalette(d.primary,palette);IDirectDrawSurface_SetPalette(d.backbuffer,palette);
 SniperSpriteSurface tiles={0};SniperSprite desc={107,0,0,16,16,8,19,0,0};hr=sniper_surface_create(d.draw,inst,&desc,&tiles);if(FAILED(hr))return 3;
 IDirectDrawSurface_SetPalette(tiles.surface,palette);sniper_surface_restore(&tiles);
 SniperEditorPainter p={0};hr=sniper_editor_painter_create(&p,d.draw,inst,palette);if(FAILED(hr))return 4;
 SniperEditorRenderState s={.cursor_x=10,.cursor_y=12,.screen_width=640,.screen_height=480,.selected_tile=1,.selection_active=1,.x1=10,.y1=12,.x2=12,.y2=14,.selection_color=7,.clipboard_color=9,.palette_background_color=13};
 for(int scene=0;scene<4;++scene){
  DDBLTFX fx={0};fx.dwSize=sizeof fx;fx.dwFillColor=0;hr=IDirectDrawSurface_Blt(d.backbuffer,NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&fx);if(FAILED(hr))return 5;
  if(scene==1){s.selection_active=0;s.clipboard_active=1;s.clipboard_width=s.clipboard_height=3;}
  if(scene==2){s.selection_active=1;s.clipboard_active=1;s.x1=0;s.y1=0;s.x2=100;s.y2=100;s.camera_x=s.camera_y=100;}
  if(scene==3){s.selection_active=0;s.clipboard_active=0;s.camera_x=s.camera_y=0;}
  hr=sniper_editor_paint(&p,&tiles,d.backbuffer,&s);if(FAILED(hr)){printf("paint%d %08lx\n",scene,(unsigned long)hr);return 6;}
  if(scene<2&&(!pixel(d.backbuffer,200,192,scene?9:7)||!pixel(d.backbuffer,208,220,scene?9:7)||!pixel(d.backbuffer,200,220,0)))return 7;
  if(!pixel(d.backbuffer,167,7,13)||!pixel(d.backbuffer,472,100,13)||!pixel(d.backbuffer,200,136,13))return 8;
  if(scene==3){
   DDSURFACEDESC lock={0};lock.dwSize=sizeof lock;hr=IDirectDrawSurface_Lock(d.backbuffer,NULL,&lock,DDLOCK_WAIT,NULL);if(FAILED(hr))return 9;
   PALETTEENTRY colors[256];IDirectDrawPalette_GetEntries(palette,0,0,256,colors);FILE *f=fopen("editor-frame.ppm","wb");if(!f)return 10;
   fprintf(f,"P6\n640 480\n255\n");for(int y=0;y<480;++y)for(int x=0;x<640;++x){PALETTEENTRY c=colors[((uint8_t *)lock.lpSurface)[y*lock.lPitch+x]];uint8_t rgb[3]={c.peRed,c.peGreen,c.peBlue};fwrite(rgb,1,3,f);}fclose(f);IDirectDrawSurface_Unlock(d.backbuffer,lock.lpSurface);
  }
  hr=IDirectDrawSurface_Flip(d.primary,NULL,DDFLIP_WAIT);if(FAILED(hr))return 11;
 }
 sniper_editor_painter_close(&p);sniper_surface_destroy(&tiles);IDirectDrawPalette_Release(palette);sniper_game_display_close(&d);DestroyWindow(w);
 puts("PASS4 real indexed editor scenes: selection,clipboard,clipped outline,palette/cursor and cleanup");return 0;
}
