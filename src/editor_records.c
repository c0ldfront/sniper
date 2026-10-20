#include "editor_records.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
int sniper_editor_records_load(SniperEditorRecords *s,const uint8_t *data,size_t bytes)
{
    size_t n=bytes/4;
    if (!s || (bytes && !data) || n>INT32_MAX) return 0;
    uint32_t *p=malloc(n ? n*4 : 1);
    if (!p) return 0;
    uint32_t sum=0;
    for(size_t i=0;i<n;i++) {
        p[i]=(uint32_t)data[4*i]|(uint32_t)data[4*i+1]<<8|
             (uint32_t)data[4*i+2]<<16|(uint32_t)data[4*i+3]<<24;
        for(size_t j=0;j<4;j++) sum+=data[4*i+j];
    }
    s->records=p;s->count=n;s->checksum=sum;s->dirty=0;
    return 1;
}
int sniper_editor_records_place(SniperEditorRecords *s,int32_t x,int32_t y,int32_t tile)
{
    if(!s) return 0;
    s->dirty=1;
    for(size_t i=0;i<s->count;i++) {
        uint32_t v=s->records[i];
        if((v&4095)!=(uint32_t)x || ((v>>12)&4095)!=(uint32_t)y) continue;
        if(tile) s->records[i]=(v&0xffffff)|((uint32_t)tile<<24);
        else {
            --s->count;
            memmove(s->records+i,s->records+i+1,(s->count-i)*4);
        }
        return 1;
    }
    if(!tile) return 1;
    if(s->count>=INT32_MAX || s->count>=SIZE_MAX/4) return 0;
    uint32_t *p=realloc(s->records,(s->count+1)*4);
    if(!p) return 0;
    s->records=p;
    p[s->count++]=((uint32_t)x&4095)|(((uint32_t)y&4095)<<12)|((uint32_t)tile<<24);
    return 1;
}
size_t sniper_editor_records_bytes(const SniperEditorRecords *s,uint8_t *data,size_t size)
{
    size_t n=s->count*4;
    if(data && size>=n) for(size_t i=0;i<s->count;i++)
        for(size_t j=0;j<4;j++) data[i*4+j]=(uint8_t)(s->records[i]>>(j*8));
    return n;
}
int sniper_editor_records_save_needed(const SniperEditorRecords *s)
{ return s && s->dirty && s->records; }
void sniper_editor_records_free(SniperEditorRecords *s)
{ if(s) {free(s->records);memset(s,0,sizeof(*s));} }
