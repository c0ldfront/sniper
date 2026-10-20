#!/usr/bin/env python3
"""Execute native restore's selection branch; intercept only API/access boundaries.
Also recover constructor filename/resource arguments from original instructions.
"""
from pathlib import Path
import re, struct
import pefile, capstone
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EAX
ROOT=Path(__file__).resolve().parents[2]
p=pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'))
mem=p.get_memory_mapped_image(); base=0x400000
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,0x200000);u.mem_write(base,mem)
u.mem_map(0x700000,0x20000)
obj=0x700000;surface=obj+0x1000;vtable=surface+0x100;restore=0x710000;load=restore+0x100
pack=lambda v:struct.pack('<I',v&0xffffffff)
rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
u.mem_write(surface,pack(vtable));u.mem_write(vtable+0x6c,pack(restore));u.mem_write(0x4353b0,pack(load))
seen=[];exists=False

def hook(u,a,n,data):
    sp=u.reg_read(UC_X86_REG_ESP)
    if a in (restore,0x41f930):
        u.reg_write(UC_X86_REG_EAX,0 if a==restore or exists else 0xffffffff)
        u.reg_write(UC_X86_REG_ESP,sp+(8 if a==restore else 4));u.reg_write(UC_X86_REG_EIP,rd(sp))
    elif a==load:
        seen.append([rd(sp+4+i*4) for i in range(6)]);u.emu_stop()
u.hook_add(UC_HOOK_CODE,hook)
for name in (b'',b'ships.bmp',b'invalid.bmp'):
 for exists in (False,True):
  for resource in (700,139,154,0x12345678):
   u.mem_write(obj,bytes(0xa8));u.mem_write(obj,pack(surface));u.mem_write(obj+4,name+b'\0');u.mem_write(obj+0x9c,pack(resource))
   sp=0x71f000;u.mem_write(sp,pack(0x710200));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,obj)
   u.emu_start(0x4103e0,0x410493,count=200)
   expected=[0,obj+4 if name and exists else resource&65535,0,0,0,0x2010 if name and exists else 0x2000]
   assert seen[-1]==expected,(seen[-1],expected)
# Extract the last two push-immediates at each explicit constructor call.
d=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
ins=list(d.disasm(mem[0x1000:0x18000],0x401000));mappings={}
for i,x in enumerate(ins):
 if x.mnemonic=='call' and x.op_str=='0x4102b0':
  args=[v.op_str for v in ins[i-12:i] if v.mnemonic=='push']
  resource=int(args[-2],16);addr=int(args[-1],16)
  name=mem[addr-base:].split(b'\0',1)[0].decode();mappings[resource]=name
source=(ROOT/'code/platform/sprite_surface.c').read_text()
actual={int(r):n for r,n in re.findall(r'case (\d+):return "([^"]+)";',source)}
assert actual==mappings,(actual,mappings)
print('PASS24 native bitmap-selection branches; all16 constructor filename/resource mappings match binary')
