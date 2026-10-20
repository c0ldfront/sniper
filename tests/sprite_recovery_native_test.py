"""Original41056e..4105b1 COM retry/recovery return contract, real x86 branch."""
import pathlib,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
put(0x610000,0x611000);put(0x61106c,0x600010);put(0x620000,0x630000)
def hook(uc,a,size,user):
    global busy
    if a not in [0x600000,0x600010,0x4103e0]:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x600000:
        events.append('blit');result=0x8876021c if busy else blit_result;busy=max(0,busy-1);pop=24
    elif a==0x600010:events.append('target');result=target_result;pop=4
    else:events.append('source');result=0x80004005;pop=0
    uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4+pop);uc.reg_write(UC_X86_REG_EIP,get(sp))
u.hook_add(UC_HOOK_CODE,hook)
count=0
for blit_result in [0,0x887601c2,0x80004005,0x88760082]:
 for target_result in [0,0x887601c2,0x80004005,0x88760082]:
  for wait in range(3):
    events=[];busy=wait
    for reg,value in [(UC_X86_REG_ESP,0x800000),(UC_X86_REG_ESI,0x620000),(UC_X86_REG_EDI,0x610000),(UC_X86_REG_EBX,0x600000),(UC_X86_REG_EBP,0x8000)]:u.reg_write(reg,value)
    put(0x800030,0x700000);u.emu_start(0x41056e,0x700000,count=1000)
    assert u.reg_read(UC_X86_REG_EAX)==(target_result if blit_result==0x887601c2 else blit_result)
    assert events==['blit']*(wait+1)+(['source','target'] if blit_result==0x887601c2 else [])
    count+=1
print(f'PASS {count} original sprite recovery traces: target Restore result replaces lost Blt HRESULT, source restore result ignored, no redraw')
