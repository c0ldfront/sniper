#include "network_checksum.h"
#include <stdio.h>
uint32_t sniper_network_checksum(const uint8_t *data,size_t size){
 uint32_t sum=0;
 for(size_t i=0;i<size/4;i++,data+=4)
  sum+=(uint32_t)data[0]|(uint32_t)data[1]<<8|(uint32_t)data[2]<<16|(uint32_t)data[3]<<24;
 return sum;
}
uint32_t sniper_network_checksum_file(const char *path){
 FILE *f=fopen(path,"rb");if(!f)return 0;
 uint32_t sum=0;uint8_t word[4];
 while(fread(word,1,4,f)==4)sum+=sniper_network_checksum(word,4);
 fclose(f);return sum;
}
