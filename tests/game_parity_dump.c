/* Pause execution, then read original ship/config/tick/pickups.
 * No process memory writes or code patches; caller terminates paused process. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
int main(void){
 HWND w=FindWindowA("SniperClass",NULL);DWORD pid=0;GetWindowThreadProcessId(w,&pid);
 HANDLE p=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION|0x0800,FALSE,pid);if(!p)return 1;
 typedef LONG (WINAPI *SuspendProcess)(HANDLE);
 SuspendProcess suspend=(SuspendProcess)(void *)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtSuspendProcess");
 if(!suspend||suspend(p)<0)return 8;
 uint32_t address,tick;SIZE_T got;unsigned char ship[0x218],config[288];
 if(!ReadProcessMemory(p,(void *)0x4218e0,&address,4,&got)||got!=4)return 2;
 if(!ReadProcessMemory(p,(void *)(uintptr_t)address,ship,sizeof ship,&got)||got!=sizeof ship)return 3;
 if(!ReadProcessMemory(p,(void *)0x4281c9,config,sizeof config,&got)||got!=sizeof config)return 4;
 if(!ReadProcessMemory(p,(void *)0x4218cc,&tick,4,&got)||got!=4)return 9;
 FILE *f=fopen("/work/original-ship.bin","wb");if(!f)return 5;fwrite(ship,1,sizeof ship,f);fwrite(config,1,sizeof config,f);fwrite(&tick,1,4,f);
 uint32_t count;if(!ReadProcessMemory(p,(void *)0x428a08,&count,4,&got)||count>16)return 10;fwrite(&count,1,4,f);
 for(uint32_t i=0;i<count;++i){uint32_t ptr;unsigned char pickup[20];
  if(!ReadProcessMemory(p,(void *)(uintptr_t)(0x429288+i*4),&ptr,4,&got))return 11;
  if(!ReadProcessMemory(p,(void *)(uintptr_t)ptr,pickup,20,&got)||got!=20)return 12;
  fwrite(pickup,1,20,f);
 }
 fclose(f);
 uint32_t width=0,height=0;
 ReadProcessMemory(p,(void *)0x42818c,&width,4,&got);ReadProcessMemory(p,(void *)0x42434c,&height,4,&got);
 printf("native viewport=%lux%lu tick=%lu pickups=%lu\n",(unsigned long)width,(unsigned long)height,(unsigned long)tick,(unsigned long)count);
 printf("PAUSED READ ONLY pid=%lu ship=%08lx x=%ld y=%ld\n",(unsigned long)pid,(unsigned long)address,*(long *)(ship+0x48),*(long *)(ship+0x4c));CloseHandle(p);return 0;
}
