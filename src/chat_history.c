#include "chat_history.h"
#include <string.h>
static void line(char *out,const char *text,const char *name,size_t limit,int space) {
    size_t at=0;
    if(name) {
        size_t n=0;while(n<10 && name[n])++n;
        for(size_t i=n;i<10;++i)out[at++]=' ';
        memcpy(out+at,name,n);at+=n;out[at++]='>';if(space)out[at++]=' ';
    }
    size_t n=0;while(n<limit && text[n])++n;
    memcpy(out+at,text,n);out[at+n]=0;
}
void sniper_chat_history(char (*lines)[128],int32_t *colors,int32_t rows,
    int32_t width,int32_t *dirty,const char *text,const char *name,int32_t recipient) {
    if(rows<3)return;
    *dirty=1;
    size_t limit=name?32u:43u;
    int split=width==320 && strlen(text)>limit;
    int added=split?2:1;
    memmove(lines+added,lines,(size_t)(rows-added)*128u);
    memmove(colors+added,colors,(size_t)(rows-added)*sizeof(*colors));
    int32_t color=recipient==-1?0:recipient==-2?2:3;
    if(split) {
        line(lines[1],text,name,limit,0);colors[1]=color;
        line(lines[0],text+limit,0,43,0);colors[0]=color;
    } else {line(lines[0],text,name,100,1);colors[0]=color;}
}
