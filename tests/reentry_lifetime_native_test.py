"""Bounded native menu reentry: full handlers plus actual constructor reset.
Win32 calls stubbed; constructor returns after reset block, before allocations.
"""
import pathlib,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
owned=[0x424348,0x4289f8,0x42dd6c,0x428a60,0x429f48,0x429f50]
trace=[];saved=None

def hook(uc,a,size,user):
 global saved
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a==0x4026c0:
  trace.append(('constructor',get(sp+4),tuple(get(x) for x in owned)))
  saved=(sp,get(sp),uc.reg_read(UC_X86_REG_EBP))
  uc.reg_write(UC_X86_REG_EBP,0x820000);put(0x820008,get(sp+4))
  uc.reg_write(UC_X86_REG_EIP,0x402709);return
 if a==0x402913:
  trace.append(('reset',tuple(get(x) for x in owned)))
  sp,ret,bp=saved;uc.reg_write(UC_X86_REG_EBP,bp);uc.reg_write(UC_X86_REG_ESP,sp+4);uc.reg_write(UC_X86_REG_EIP,ret);uc.reg_write(UC_X86_REG_EAX,success);return
 if a==0x403880:trace.append(('cleanup',));result=0;extra=0
 elif a==0x700000:result=0x8000 if shortcut else 0;extra=4
 elif a==0x700010:trace.append(('dialog',accepted));result=accepted;extra=20
 elif a==0x700020:result=99;extra=8
 elif a==0x700030:result=99;extra=4
 else:return
 uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4+extra);uc.reg_write(UC_X86_REG_EIP,get(sp))
u.hook_add(UC_HOOK_CODE,hook)
for a,v in [(0x4354b8,0x700000),(0x435568,0x700010),(0x4354d8,0x700020),(0x4354dc,0x700030)]:put(a,v)
count=0
for active in [0,1]:
 for direct,shortcut,accepted in [(0,0,0),(0,0,1),(0,1,0),(1,0,0)]:
  for success in [0,1]:
   u.mem_write(0x42def8,b'pilot\0');u.mem_write(0x42e150,b'\0');u.mem_write(0x4222b4,b'\0');put(0x42223c,active)
   before=tuple(0x600000+4*i if active else 0 for i in range(len(owned)))
   for a,v in zip(owned,before):put(a,v)
   put(0x42227c,4);u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EBP,0x810000);trace.clear()
   u.emu_start(0x40bde9 if direct else 0x40bbf8,0x40cddb,count=10000)
   canceled=not direct and not shortcut and not accepted
   if canceled:
    assert trace==[('dialog',0)] and tuple(get(x) for x in owned)==before and get(0x42223c)==active
   else:
    expected=[] if direct or shortcut else [('dialog',1)]
    expected += [('constructor',int(bool(direct or shortcut)),before),('reset',(0,)*len(owned))]
    if not success:expected += [('cleanup',)]
    assert trace==expected,(active,direct,shortcut,accepted,success,trace)
    assert get(0x42223c)==success
   count+=1
print(f'PASS{count} native reentry paths: cancel retains old ownership; accepted ordinary/editor overwrites ownership before any cleanup; cleanup only after constructor failure')
