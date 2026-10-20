#include "chat_dispatch.h"
#include <limits.h>
#include <string.h>
static int32_t integer(const unsigned char *p) {
    while(*p==' ' || (*p>=9 && *p<=13))++p;
    int negative=*p=='-';if(*p=='-' || *p=='+')++p;
    uint32_t value=0;while(*p>='0' && *p<='9')value=value*10u+(*p++-'0');
    if(negative)value=0u-value;
    int32_t result;memcpy(&result,&value,4);return result;
}
void sniper_chat_dispatch(SniperChatDispatch *c,const char *text) {
    while(*text==' ')++text;
    if(!*text)return;
    if(*text=='=') {
        int32_t n=integer((const unsigned char *)text+1);
        if(!c->team_modulus || (n==INT32_MIN && c->team_modulus==-1))return;
        int32_t frequency=n%c->team_modulus, count=0;
        for(int32_t i=0;i<c->count;++i)if(c->ordered[i]->word[0x68/4]==frequency)++count;
        if(count>=c->team_capacity){sniper_notice_push(c->notices,"Team is full",400,2);return;}
        *c->requested_team=frequency;c->team(c->user,frequency);
        if(c->local->word[0x6c/4]!=-1)c->attach(c->user,-1);
    } else if(*text=='\'')c->send(c->user,-2,text+1);
    else if(*text=='/') {
        if(text[1]=='/')c->send(c->user,-2,text+2);
        else if(c->selected>=0 && c->selected<c->count)
            c->send(c->user,c->ordered[c->selected]->word[0x44/4],text+1);
    } else c->send(c->user,-1,text);
}
