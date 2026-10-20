/* Run a private in-memory copy of original speed-limit instructions on x87.
 * No original executable startup, imports, networking or UI are executed. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "flight_math.h"
static void original_limit(int maximum) {
    __asm__ volatile("movl %0,%%edi; subl $32,%%esp; movl $0x200062a6,%%eax; call *%%eax; addl $32,%%esp"
        : : "m"(maximum) : "eax","ebx","ecx","edx","esi","edi","memory","cc");
}
static uint32_t random_state=87;
static uint32_t random_word(void) { random_state=random_state*1664525u+1013904223u;return random_state; }
int main(void) {
    FILE *f=fopen("mapped-original.bin","rb");
    if (!f) return 2;
    void *image=VirtualAlloc((void*)0x20000000,0x100000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if (image!=(void*)0x20000000) { printf("allocation failed error=%lu module=%p\n",GetLastError(),(void*)GetModuleHandleA(NULL)); return 3; }
    size_t size=fread(image,1,0x100000,f);fclose(f);
    if(size<0x35584) return 4;
    /* Rebase only absolute global operands in the selected block. Relative
     * CRT calls retain their original displacement in the mapped image. */
    for(uint8_t *p=(uint8_t*)0x200062a6;p<(uint8_t*)0x20006332-3;++p) {
        uint32_t v; memcpy(&v,p,4);
        if(v==0x4218e0 || v==0x4230fc) { v+=0x1fc00000;memcpy(p,&v,4); }
    }
    /* Stop at the exact end of the isolated block; original file is untouched. */
    *(unsigned char*)0x20006332=0xc3;
    int32_t ship[128]={0};
    *(int32_t**)0x200218e0=ship;
    *(int32_t*)0x200230fc=0;
    unsigned count=0;
    for(unsigned precision=0;precision<2;++precision) {
        unsigned short cw=precision ? 0x27f : 0x37f;
        __asm__ volatile("fldcw %0" : : "m"(cw));
        for(unsigned i=0;i<20001;++i) {
            int32_t vx=i ? (int32_t)(random_word()%40001)-20000 : 3000;
            int32_t vy=i ? (int32_t)(random_word()%40001)-20000 : 4000;
            int32_t max=i ? random_word()%10001 : 10;
            ship[0x50/4]=vx;ship[0x54/4]=vy;
            original_limit(max);
            int32_t x=vx,y=vy;
            sniper_flight_limit(max,&x,&y);
            if(x!=ship[0x50/4] || y!=ship[0x54/4]) {
                printf("FAIL cw=%x in=%d,%d max=%d original=%d,%d recovered=%d,%d\n",cw,vx,vy,max,ship[0x50/4],ship[0x54/4],x,y);
                return 1;
            }
            if(!i) printf("Boundary cw=%x input=3000,4000 max=10 original=%d,%d recovered=%d,%d\n",cw,ship[20],ship[21],x,y);
            ++count;
        }
    }
    printf("%u speed-limit cases match original instructions on hardware x87 (53/64-bit precision)\n",count);
    return 0;
}
