#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "flight_math.h"
#include "numeric_environment.h"
void native_accelerate(int32_t);
__asm__(".text\n.globl _native_accelerate\n_native_accelerate:\n"
        "pushl %ebp\npushl %ebx\npushl %esi\npushl %edi\nsubl $32,%esp\n"
        "movl 52(%esp),%ebx\nmovl $1,%esi\nmovl $0x200061d5,%eax\ncall *%eax\n"
        "addl $32,%esp\npopl %edi\npopl %esi\npopl %ebx\npopl %ebp\nret\n");
static uint16_t control(void){uint16_t v;__asm__ volatile("fnstcw %0":"=m"(v));return v;}
static void set(uint16_t v){__asm__ volatile("fldcw %0"::"m"(v));}
int main(void){
    uint16_t initial=control();printf("MinGW startup control=%04x\n",initial);
    FILE*f=fopen("mapped-original.bin","rb");if(!f)return 2;
    void*p=VirtualAlloc((void*)0x20000000,0x100000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if(p!=(void*)0x20000000)return 3;
    size_t bytes=fread(p,1,0x100000,f);fclose(f);if(bytes<0x1ca00)return 4;
    ((void(*)(void))0x2001a5c0)();uint16_t native=control();
    printf("Actual native41a5c0 control=%04x\n",native);if((native&0x300)!=0x200)return 5;
    set(initial);sniper_numeric_initialize();if(control()!=native)return 6;
    for(uint8_t*q=(uint8_t*)0x200061d5;q<(uint8_t*)0x20006235-3;++q){
        uint32_t v;memcpy(&v,q,4);
        if(v==0x4218e0||v==0x4218c8||v==0x420000){v+=0x1fc00000;memcpy(q,&v,4);}
    }
    *(uint8_t*)0x20006235=0xc3;int32_t ship[134]={0};
    *(int32_t**)0x200218e0=ship;*(int32_t*)0x200218c8=1;
    unsigned differences=0;uint32_t state=87;
    for(unsigned i=0;i<200000;++i){
        state=state*1664525u+1013904223u;int32_t heading=(int32_t)(state%40000);
        state=state*1664525u+1013904223u;int32_t acceleration=(int32_t)(state%2000000000u);
        int32_t x64=0,y64=0,x53=0,y53=0;
        set(0x37f);sniper_flight_accelerate(heading,1,acceleration,1,&x64,&y64);
        set(0x27f);sniper_flight_accelerate(heading,1,acceleration,1,&x53,&y53);
        ship[0x58/4]=heading;ship[0x50/4]=ship[0x54/4]=0;native_accelerate(acceleration);
        if(x53!=ship[0x50/4]||y53!=ship[0x54/4]){printf("FAIL native53 heading=%d amount=%d\n",heading,acceleration);return 7;}
        if(x64!=x53||y64!=y53){if(differences<4)printf("Different acceleration heading=%d amount=%d:64=%d,%d 53=%d,%d\n",heading,acceleration,x64,y64,x53,y53);++differences;}
    }
    printf("Precision-sensitive acceleration outputs=%u/200000\n",differences);
    puts("PASS200000 native53 acceleration comparisons and startup precision initializer");
    set(initial);return 0;
}
