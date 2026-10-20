#!/usr/bin/env python3
"""Original dialog callback with Win32/MCI calls intercepted, no audio hardware."""
from pathlib import Path
import struct,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
p=pefile.PE(str(Path(__file__).resolve().parents[2]/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x700000,0x20000)
pack=lambda *v:struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v])
rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,256)).split(b'\0')[0].decode()
imports={};counts={'GetDlgItem':2,'SetWindowTextA':2,'GetWindowTextA':3,'EndDialog':2,'PostMessageA':4,'SetDlgItemTextA':3,'KillTimer':2,'SetTimer':4,'mciSendStringA':4,'LoadCursorA':2,'SetCursor':1,'PlaySoundA':3}
for entry in p.DIRECTORY_ENTRY_IMPORT:
 for imp in entry.imports:
  name=imp.name.decode() if imp.name else ''
  if name in counts:
   addr=0x710000+len(imports)*16;imports[addr]=name;u.mem_write(imp.address,pack(addr))
events=[]
def hook(u,a,n,d):
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x71ff00:u.emu_stop();return
 if a==0x416bc0:
  dest,fmt,slot=[rd(sp+4+i*4) for i in range(3)];u.mem_write(dest,(string(fmt)%slot).encode()+b'\0');u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4);return
 if a not in imports:return
 name=imports[a];args=[rd(sp+4+i*4) for i in range(counts[name])];result=1
 if name=='GetDlgItem':result=args[1]
 elif name=='SetWindowTextA':events.append(('set',args[0],string(args[1])))
 elif name=='GetWindowTextA':u.mem_write(args[1],('edited'+str(args[0])).encode()+b'\0');events.append(('get',args[0],args[2]))
 elif name=='mciSendStringA':events.append(('mci',string(args[0])))
 else:events.append((name,*args))
 u.reg_write(UC_X86_REG_EAX,result);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4+4*len(args))
u.hook_add(UC_HOOK_CODE,hook)
def run(msg,wp=0,record=0):
 events.clear();u.mem_write(0x4222ac,pack(record));sp=0x71e000;u.mem_write(sp,pack(0x71ff00,999,msg,wp,0));u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(0x40faa0,0x71ff00,count=3000);return events[:]
ids=[1020,1021,1022,1026,1024,1025,1023,1027]
for i in range(8):u.mem_write(0x42e2a8+i*82,('macro'+str(i)).encode()+b'\0')
assert run(0x110)==[('set',n,'macro'+str(i)) for i,n in enumerate(ids)]
assert run(0x111,1)==[('get',n,82) for n in ids]+[('PostMessageA',999,16,1,0)]
assert run(0x111,2)==[('PostMessageA',999,16,0,0)]
for slot,button in enumerate([1028,1030]):
 e=run(0x111,button);assert ('SetTimer',999,1,3000,0) in e;assert [x[1] for x in e if x[0]=='mci']==['open new type waveaudio alias capture','record capture'];assert rd(0x4222ac)==slot+1
 for msg,wp in [(0x113,99),(0x111,1),(0x111,2),(0x111,12345)]:
  e=run(msg,wp,slot+1);assert [x[1] for x in e if x[0]=='mci']==['stop capture',f'save capture record{slot}.wav','close capture'];assert not any(x[0]=='PostMessageA' for x in e);assert rd(0x4222ac)==0
assert run(16,7)==[('EndDialog',999,7)]
print('PASS14 native macro-dialog paths: eight field mappings,82-byte reads,OK/Cancel,record/timer/command stop and close')
