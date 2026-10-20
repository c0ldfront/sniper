#include "chat_render.h"
#include <string.h>
#include <limits.h>
static int32_t signed_bits(uint32_t value) { int32_t result;memcpy(&result,&value,4);return result; }
int sniper_chat_layout(int32_t screen_width,int32_t rows,SniperChatLayout *out) {
    if(!out||rows<3||rows>INT32_MAX/8)return 0;
    out->width=screen_width==320?267:534;
    out->height=rows*8;out->max_chars=out->width/6-1;return 1;
}
void sniper_chat_history_draw(const SniperChatLayout *layout,const char (*lines)[128],const int32_t *colors,int32_t rows,SniperChatText draw,void *user) {
    if(!layout||!lines||!colors||!draw||rows<3)return;
    int32_t y=layout->height-8;
    for(int32_t i=0;i<rows;i++) {
        char text[128];size_t count=0;
        while(count<127&&lines[i][count]&&count<(size_t)layout->max_chars)count++;
        memcpy(text,lines[i],count);text[count]=0;
        if(text[0]) {draw(user,text,0,y,colors[i]);y-=8;}
    }
}
void sniper_chat_placement(int32_t screen_height,int32_t width,int32_t height,const char *input,SniperChatRect *destination,SniperChatText draw,void *user) {
    int32_t bottom=signed_bits((uint32_t)screen_height-1);
    if(input&&input[0]) {bottom=signed_bits((uint32_t)bottom-8);if(draw)draw(user,input,1,bottom,1);}
    if(destination) {destination->left=1;destination->top=signed_bits((uint32_t)bottom-(uint32_t)height);destination->right=signed_bits((uint32_t)width+1);destination->bottom=bottom;}
}
int sniper_chat_ticker_due(uint32_t ms,uint32_t last,int32_t interval,int32_t enabled) {
    return signed_bits(ms/10-last)>interval&&enabled!=0;
}
