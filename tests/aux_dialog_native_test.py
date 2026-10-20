#!/usr/bin/env python3
"""Original squad and viewer callbacks with external APIs intercepted."""
from pathlib import Path
import struct,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
p=pefile.PE(str(Path(__file__).resolve().parents[2]/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x700000,0x20000)
pk=lambda *v:struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]);rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
def text(a):return bytes(u.mem_read(a,256)).split(b'\0')[0].decode()
imports={};counts={'GetDlgItem':2,'SetWindowTextA':2,'GetWindowTextA':3,'PostMessageA':4,'EndDialog':2,'MoveWindow':6,'SendMessageA':4,'GetPrivateProfileIntA':4,'GetWindowRect':2,'WritePrivateProfileStringA':4}
for dll in p.DIRECTORY_ENTRY_IMPORT:
 for imp in dll.imports:
  name=imp.name.decode() if imp.name else ''
  if name in counts:
   a=0x710000+len(imports)*16;imports[a]=name;u.mem_write(imp.address,pk(a))
events=[]
def hook(u,a,n,d):
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x71ff00:u.emu_stop();return
 if a in (0x4165c0,0x416bc0):
  if a==0x416bc0:
   dst,fmt,v=[rd(sp+4+i*4) for i in range(3)];u.mem_write(dst,(text(fmt)%(v if v<2**31 else v-2**32)).encode()+b'\0')
  u.reg_write(UC_X86_REG_EAX,0);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4);return
 if a not in imports:return
 name=imports[a];args=[rd(sp+4+i*4) for i in range(counts[name])];r=1
 if name=='GetDlgItem':r=args[1]
 elif name=='SetWindowTextA':events.append(('set',args[0],text(args[1])))
 elif name=='GetWindowTextA':events.append(('get',args[0],args[2]));u.mem_write(args[1],b'accepted\0')
 elif name=='GetPrivateProfileIntA':events.append(('read',text(args[0]),text(args[1]),args[2]));r=args[2]
 elif name=='WritePrivateProfileStringA':events.append(('write',text(args[0]),text(args[1]),text(args[2])))
 elif name=='GetWindowRect':u.mem_write(args[1],pk(30,40,380,280))
 else:events.append((name,*args))
 u.reg_write(UC_X86_REG_EAX,r);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4+len(args)*4)
u.hook_add(UC_HOOK_CODE,hook)
def run(fn,msg,wp=0,lp=0):
 events.clear();sp=0x71e000;u.mem_write(sp,pk(0x71ff00,999,msg,wp,lp));u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(fn,0x71ff00,count=10000);return events[:]
assert run(0x40e100,0x110)==[('set',1036,''),('set',1037,'')]
assert run(0x40e100,0x111,1)==[('get',1036,32),('get',1037,32),('PostMessageA',999,16,1,0)]
assert run(0x40e100,0x111,2)==[('PostMessageA',999,16,0,0)]
assert run(0x40e100,16,7)==[('EndDialog',999,7)]
e=run(0x40f810,0x110);assert e==[('read','ViewFile','X',0xffffffff),('read','ViewFile','Y',0xffffffff),('read','ViewFile','Width',150),('read','ViewFile','Height',100),('SendMessageA',1018,177,0,0)],e
assert run(0x40f810,5,0,(200<<16)|300)==[('MoveWindow',1018,0,0,300,200,1)]
for command in (1,2):assert run(0x40f810,0x111,command)==[('PostMessageA',999,16,0,0)]
assert run(0x40f810,0x111,1018)==[('SendMessageA',1018,177,0,0)]
assert run(0x40f810,16)==[('write','ViewFile',k,v) for k,v in zip(['X','Y','Width','Height'],['30','40','350','240'])]+[('EndDialog',999,0)]
print('PASS10 native auxiliary dialog paths: squad init/OK/Cancel/close;viewer missing-file init/resize/commands/profile close')
