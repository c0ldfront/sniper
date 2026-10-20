#include "editor_input.h"
#include <stdlib.h>
#include <string.h>
static int32_t add(int32_t a,int32_t b){uint32_t v=(uint32_t)a+(uint32_t)b;int32_t r;memcpy(&r,&v,4);return r;}
static int inside(int32_t x,int32_t y){return x>=0&&y>=0&&x<1024&&y<1024;}
static int held(int32_t x){return (x&0x8000)!=0;}
static int32_t minimum(int32_t a,int32_t b){return a<b?a:b;}
void sniper_editor_close(SniperEditor *e){free(e->clipboard);e->clipboard=NULL;e->clipboard_width=e->clipboard_height=0;}
int sniper_editor_input(SniperEditor *e,SniperEditorInput *i,uint8_t *map,const SniperTileSize sizes[256],const SniperEditorHost *h){
    int32_t dx=0,dy=0,paint=-1;
    if((uint32_t)i->repeat&0x40000000u){
        if(held(h->async_key(h->user,37)))dx=-1;
        if(held(h->async_key(h->user,39)))dx=1;
        if(held(h->async_key(h->user,38)))dy=-1;
        if(held(h->async_key(h->user,40)))dy=1;
        if(i->key>=37&&i->key<=40)i->key=0;
    }
    switch(i->key){case 37:dx=-1;break;case 38:dy=-1;break;case 39:dx=1;break;case 40:dy=1;break;}
    if(held(h->event_key(h->user,16))){
        int32_t t=add(add(e->tile,dy*19),dx-1);if(t>=0&&t<152)e->tile=t+1;
    }else{
        int32_t x=add(e->x,dx),y=add(e->y,dy);
        if(x>=0&&x<1024)e->x=x;
        if(y>=0&&y<1024)e->y=y;
        if(e->selection){e->x2=e->x;e->y2=e->y;}
    }
    if(!inside(e->x,e->y))return 0;
    if(held(h->async_key(h->user,17))&&map[e->y*1024+e->x]!=250)paint=e->tile;
    if(i->character>='A'&&i->character<='D'){paint=i->character+151;goto place;}
    if(i->character=='e'){
        int32_t x=e->x,y=e->y,w=e->clipboard_width,hh=e->clipboard_height;
        if(e->selection){
            x=minimum(e->x1,e->x2);y=minimum(e->y1,e->y2);
            if(!inside(e->x1,e->y1)||!inside(e->x2,e->y2))return 0;
            w=abs(e->x2-e->x1)+1;hh=abs(e->y2-e->y1)+1;
        }
        if(e->selection||e->clipboard)for(int32_t row=0;row<hh;++row)for(int32_t col=0;col<w;++col){
            int32_t xx=add(x,col),yy=add(y,row);
            if(xx<1024&&yy<1024){
                int tile=inside(xx,yy)?map[yy*1024+xx]:20;
                if(tile!=0&&tile!=250)h->place(h->user,xx,yy,0);
            }
        }
    }else if(i->character=='p'&&e->clipboard){
        const uint8_t *c=e->clipboard;
        for(int32_t row=0;row<e->clipboard_height;++row)for(int32_t col=0;col<e->clipboard_width;++col){
            int tile=*c++;int32_t x=add(e->x,col),y=add(e->y,row);
            if(tile>=1&&tile<250&&inside(x,y)&&map[y*1024+x]==0)h->place(h->user,x,y,tile);
        }
    }else if(i->character=='s'){
        if(e->selection){
            e->selection=0;
            if(!inside(e->x1,e->y1)||!inside(e->x2,e->y2))return 0;
            e->clipboard_width=abs(e->x2-e->x1)+1;e->clipboard_height=abs(e->y2-e->y1)+1;
            uint8_t *copy=malloc((size_t)e->clipboard_width*e->clipboard_height);if(!copy)return 0;
            e->clipboard=copy;
            int32_t x=minimum(e->x1,e->x2),y=minimum(e->y1,e->y2);
            for(int row=0;row<e->clipboard_height;++row)memcpy(copy+row*e->clipboard_width,map+(y+row)*1024+x,e->clipboard_width);
        }else if(!e->clipboard){e->x1=e->x2=e->x;e->y1=e->y2=e->y;e->selection=1;}
        else sniper_editor_close(e);
    }
    switch(i->key){
        case 13:case 115:i->radar_dirty=1;i->radar=i->radar==0;break;
        case 27:h->exit(h->user);break;
        case 32:if(map[e->y*1024+e->x]==0)paint=e->tile;else if(map[e->y*1024+e->x]!=250)paint=0;break;
        case 46:paint=0;break;
    }
place:
    if(paint>=216){
        if(paint>=256)return 0;
        int32_t width=sizes[paint].width/16,height=sizes[paint].height/16;
        for(int32_t x=0;x<width;++x)for(int32_t y=0;y<height;++y){
            int32_t xx=add(e->x,x),yy=add(e->y,y);
            if(!inside(xx,yy)||map[yy*1024+xx]!=0){paint=-1;break;}
        }
    }
    if(paint!=-1)h->place(h->user,e->x,e->y,paint);
    return 1;
}
