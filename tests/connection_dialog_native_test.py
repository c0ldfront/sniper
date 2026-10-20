#!/usr/bin/env python3
"""Execute native command acceptance; retain oracle for Wine dialog comparison."""
from pathlib import Path
import struct,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=Path(__file__).resolve().parents[2];p=pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x700000,0x20000)
pk=lambda *v:struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]);rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
def text(a):return bytes(u.mem_read(a,256)).split(b'\0')[0].decode()
imports={};counts={'GetDlgItem':2,'GetWindowTextA':3,'PostMessageA':4,'MessageBoxA':4,'EndDialog':2}
for entry in p.DIRECTORY_ENTRY_IMPORT:
 for imp in entry.imports:
  name=imp.name.decode() if imp.name else ''
  if name in counts:
   a=0x710000+len(imports)*16;imports[a]=name;u.mem_write(imp.address,pk(a))
fields={};result=0

def hook(u,a,n,d):
 global result
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x71ff00:u.emu_stop();return
 if a==0x416bc0:
  dest,fmt,value=[rd(sp+4+i*4) for i in range(3)];u.mem_write(dest,(text(fmt)%value).encode()+b'\0');u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4);return
 if a not in imports:return
 name=imports[a];args=[rd(sp+4+i*4) for i in range(counts[name])];r=1
 if name=='GetDlgItem':r=args[1]
 elif name=='GetWindowTextA':s=fields[args[0]][:args[2]-1];u.mem_write(args[1],s.encode()+b'\0');r=len(s)
 elif name=='PostMessageA':result=args[2]
 elif name=='MessageBoxA':result=-1
 u.reg_write(UC_X86_REG_EAX,r);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4+len(args)*4)
u.hook_add(UC_HOOK_CODE,hook)
rows=[]
for i in range(24):
 name=['Pilot',' Pilot','  Pilot','Pilot  ','x'*40,'A B'][i%6];arena=['Warzone','warzone','127.0.0.1,4777','example.test','localhost','127.0.0.1,'][i%6];quick=i%2
 fields={1008:name,1007:'password',1034:arena};result=0
 u.mem_write(0x42e150,bytes(80));u.mem_write(0x42def8,bytes(264));u.mem_write(0x42e540,bytes(152));u.mem_write(0x42e540,b'Warzone\0');u.mem_write(0x42e590,b'205.214.55.45\0');u.mem_write(0x42e5d0,b'382\0');u.mem_write(0x42f840,pk(1))
 sp=0x71e000;u.mem_write(sp,pk(0x71ff00,999,0x111,1033 if quick else 1,0));u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(0x40dbf0,0x71ff00,count=20000)
 row=[name,arena,str(quick),str(result),text(0x42e178),text(0x42def8),text(0x42e150)];rows.append('\t'.join(row))
 assert result==(-1 if arena=='localhost' else 2 if quick else 1)
 assert text(0x42e178)==(['Pilot','Pilott','Pilottt','Pilot','x'*31,'A B'][i%6])
out=root/'research/sniper87/runtime-bitmap-overrides/connection-oracle.tsv';out.write_text('\n'.join(rows)+'\n')
print('PASS24 native connection cases; oracle retained:',out)
