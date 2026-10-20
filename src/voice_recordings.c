#include "voice_recordings.h"
#include "network_compression.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
void sniper_voice_recordings_close(SniperVoiceRecordings *s) {
    for(int i=0;i<2;++i){free(s->data[i]);s->data[i]=NULL;s->length[i]=0;}
}
void sniper_voice_recordings_load(SniperVoiceRecordings *s,const char *directory) {
    sniper_voice_recordings_close(s);
    if(!directory)directory=".";
    size_t n=strlen(directory);if(n>SIZE_MAX-14)return;
    char *path=malloc(n+14);if(!path)return;
    for(int slot=0;slot<2;++slot) {
        snprintf(path,n+14,"%s/record%d.wav",directory,slot);
        FILE *f=fopen(path,"rb");if(!f)continue;
        if(fseek(f,0,SEEK_END)){fclose(f);continue;}
        long size=ftell(f);
        if(size<0||size>INT32_MAX||fseek(f,0,SEEK_SET)){fclose(f);continue;}
        uint8_t *raw=malloc(size?(size_t)size:1);
        if(!raw){fclose(f);continue;}
        if(fread(raw,1,(size_t)size,f)==(size_t)size)
            s->data[slot]=sniper_network_compress(raw,(int32_t)size,&s->length[slot],0,2);
        free(raw);fclose(f);
    }
    free(path);
}
