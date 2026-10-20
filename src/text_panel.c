#include "text_panel.h"
#include "font.h"
#include <limits.h>
int sniper_text_panel_layout(const char *title,const SniperPanelRow *rows,int32_t count,
    int32_t per,SniperTextPanelLayout *out) {
    if(!title||!rows||!out||count<=0||count>256||per<=0)return 0;
    int32_t maximum=0;
    for(int32_t i=0;i<count;++i){if(!rows[i].text)return 0;int32_t w=sniper_font_width(rows[i].text);if(w>maximum)maximum=w;}
    int32_t columns=1+(count-1)/per;
    if(maximum>INT_MAX-5 || (int64_t)columns*(maximum+5)+1>INT_MAX)return 0;
    *out=(SniperTextPanelLayout){columns*(maximum+5)+1,(count<per?count:per)*8+6+(*title?12:0),columns,maximum+5};
    return 1;
}
void sniper_text_panel_draw(const char *title,const SniperPanelRow *rows,int32_t count,
    int32_t per,const SniperTextPanelLayout *l,const SniperTextPanelDraw *d) {
    d->line(d->user,0,0,0,l->width);d->line(d->user,0,0,l->height-1,l->width);
    d->line(d->user,1,0,1,l->height-2);d->line(d->user,1,l->width-1,1,l->height-2);
    if(*title)d->line(d->user,0,0,12,l->width);
    for(int32_t i=1;i<l->columns;++i)d->line(d->user,1,i*l->stride+1,15,l->height-15);
    if(*title)d->text(d->user,title,(l->width-sniper_font_width(title))/2,3,3);
    for(int32_t i=0;i<count;++i)d->text(d->user,rows[i].text,(i/per)*l->stride+3,(i%per)*8+15,rows[i].color);
}
