#include "network_compression.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
typedef struct Node { uint32_t count; int left,right; uint8_t symbol; } Node;
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put32(uint8_t *p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int tree(Node *n,const uint32_t *hist){int q[256],used=0,len=0;
 for(int i=0;i<256;i++)if(hist[i]){n[used]=(Node){hist[i],-1,-1,(uint8_t)i};q[len++]=used++;}
 if(!len)return -1;
 while(len>1){int child[2];for(int k=0;k<2;k++){int m=0;for(int j=1;j<len;j++)if(n[q[m]].count>n[q[j]].count)m=j;
 child[k]=q[m];memmove(q+m,q+m+1,(size_t)(--len-m)*sizeof(*q));}
 n[used]=(Node){n[child[0]].count+n[child[1]].count,child[0],child[1],0};q[len++]=used++;}return q[0];}
static void codes(Node *n,int at,uint32_t code,unsigned depth,uint32_t *bits,unsigned *length){
 if(n[at].left<0){bits[n[at].symbol]=code;length[n[at].symbol]=depth;return;}
 codes(n,n[at].left,code*2,depth+1,bits,length);codes(n,n[at].right,code*2+1,depth+1,bits,length);}
uint8_t *sniper_network_compress(const uint8_t *input,int32_t size,int32_t *out_size,int bypass,uint32_t passes){
 if(!out_size||size<0||(!input&&size))return NULL;
 const uint8_t *current=input;uint8_t *owned=NULL;uint32_t count=0;
 while(!(uint8_t)bypass&&count<passes&&size){uint32_t hist[256]={0},bits[256]={0};unsigned lengths[256]={0};Node nodes[511];
 for(int32_t i=0;i<size;i++)hist[current[i]]++;
 int root=tree(nodes,hist);codes(nodes,root,0,0,bits,lengths);uint64_t total=0;
 for(int i=0;i<256;i++){if(lengths[i]>25){free(owned);return NULL;}total+=(uint64_t)hist[i]*lengths[i];}
 uint64_t next_size=772+(total+7)/8;if(next_size>=(uint32_t)size)break;
 uint8_t *next=calloc((size_t)next_size,1);if(!next){free(owned);return NULL;}
 for(int i=0;i<256;i++){next[i*3]=hist[i]>>16;next[i*3+1]=hist[i]>>8;next[i*3+2]=hist[i];}put32(next+768,(uint32_t)total);
 uint64_t pos=0;for(int32_t i=0;i<size;i++)for(unsigned k=lengths[current[i]];k;k--,pos++)if((bits[current[i]]>>(k-1))&1)next[772+pos/8]|=128>>(pos%8);
 free(owned);owned=next;current=next;size=(int32_t)next_size;count++;
 }
 if(size>INT32_MAX-4){free(owned);return NULL;}uint8_t *result=malloc((size_t)size+4);if(!result){free(owned);return NULL;}
 put32(result,count);if(size)memcpy(result+4,current,(size_t)size);free(owned);*out_size=size+4;return result;
}
uint8_t *sniper_network_decompress(const uint8_t *input,int32_t size,int32_t *out_size){
 if(!out_size||!input||size<4)return NULL;
 uint32_t passes=be32(input);size-=4;
 uint8_t *current=malloc(size?(size_t)size:1);if(!current)return NULL;if(size)memcpy(current,input+4,(size_t)size);
 while(passes--){if(size<772)goto fail;uint32_t hist[256];Node nodes[511];for(int i=0;i<256;i++)hist[i]=(uint32_t)current[i*3]<<16|(uint32_t)current[i*3+1]<<8|current[i*3+2];
 uint32_t total=be32(current+768);if((uint64_t)total>(uint64_t)(size-772)*8)goto fail;int root=tree(nodes,hist);if(total&&(root<0||nodes[root].left<0))goto fail;
 /* At most one output byte per encoded bit. Native grows in8192-byte chunks. */
 if(total>INT32_MAX)goto fail;
 uint8_t *next=malloc(total?total:1);if(!next)goto fail;uint32_t pos=0,n=0;
 while(pos<total){int at=root;while(nodes[at].left>=0){if(pos>=total){free(next);goto fail;}at=(current[772+pos/8]&(128>>(pos%8)))?nodes[at].right:nodes[at].left;pos++;}next[n++]=nodes[at].symbol;}
 free(current);current=next;size=(int32_t)n;
 }*out_size=size;return current;
 fail:free(current);return NULL;
}
uint8_t *sniper_network_compress_callback(void *user,const uint8_t *input,int32_t size,int32_t *out_size){(void)user;return sniper_network_compress(input,size,out_size,0,1);}
