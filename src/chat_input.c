#include "chat_input.h"
#include <string.h>
int sniper_chat_input(SniperChatInput *s,int32_t width,uint8_t ch,
    int32_t key,int32_t shift,void (*submit)(void *,const char *),void *user) {
    size_t n=strlen(s->text);
    if(ch>=32&&ch<127&&n<(width==320?43u:76u)) {
        s->text[n++]=(char)ch;s->text[n]=0;
    }
    if((shift&0x8000)&&key>=0x70&&key<=0x77) {
        if(s->text[0]){s->text[80]=0;strcpy(s->macros[key-0x70],s->text);}
        if(submit)submit(user,s->macros[key-0x70]);
        s->text[0]=0;return 1;
    }
    if(key==8&&n)s->text[n-1]=0;
    if(key==13&&s->enabled&&s->text[0]) {
        if(submit)submit(user,s->text);
        s->text[0]=0;
    }
    if(key==0x74){s->enabled=!s->enabled;s->dirty=1;}
    return 0;
}
