#!/usr/bin/env python3
"""Execute original startup seed/default profile prefix with actual CRT RNG."""
from pathlib import Path
import ctypes,struct,random,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=Path(__file__).resolve().parents[2];p=pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE'))
lib=ctypes.CDLL(str(root/'code/build/libstartup_random.so'))
class State(ctypes.Structure):_fields_=[('crt_state',ctypes.c_uint32),('frequency',ctypes.c_int32)]
lib.sniper87_startup_random_seed.argtypes=[ctypes.POINTER(State),ctypes.c_uint32]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x700000,0x20000)
pk=lambda *v:struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]);rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
def text(a):return bytes(u.mem_read(a,256)).split(b'\0')[0].decode()
imports={};counts={'GetPrivateProfileIntA':4,'GetPrivateProfileStringA':6}
for dll in p.DIRECTORY_ENTRY_IMPORT:
 for imp in dll.imports:
  name=imp.name.decode() if imp.name else ''
  if name in counts:a=0x710000+len(imports)*16;imports[a]=name;u.mem_write(imp.address,pk(a))
events=[];epoch=0;saved=None

def hook(u,a,n,d):
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x40d12d:u.emu_stop();return
 if a==0x417db0:
  assert rd(sp+4)==0;events.append('time');r=epoch;pop=4
 elif a==0x416bc0:
  dst,fmt,value=[rd(sp+4+i*4) for i in range(3)];u.mem_write(dst,(text(fmt)%value).encode()+b'\0');r=0;pop=4
 elif a in imports:
  name=imports[a];v=[rd(sp+4+i*4) for i in range(counts[name])];pop=4+len(v)*4;r=0
  if name=='GetPrivateProfileStringA':u.mem_write(v[3],b'\0')
  else:
   key=text(v[1]);r=v[2]
   if key=='PlayerShipFrequency':events.append(('frequency_default',v[2]));r=v[2] if saved is None else saved
 else:return
 u.reg_write(UC_X86_REG_EAX,r&0xffffffff);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+pop)
u.hook_add(UC_HOOK_CODE,hook);rng=random.Random(4165)
for case in range(1200):
 epoch=[0,1,0xffffffff,0x80000000][case%4] if case<16 else rng.getrandbits(32);saved=None if case%3==0 else rng.choice([-1,0,1,3,9,2147483647,-2147483648])
 u.mem_write(0x422f78,pk(0xdeadbeef));events.clear();u.reg_write(UC_X86_REG_ESP,0x71e000);u.emu_start(0x40d070,0x40d12d,count=1000)
 state=State();lib.sniper87_startup_random_seed(ctypes.byref(state),epoch)
 assert rd(0x422f78)==state.crt_state
 assert events==['time',('frequency_default',state.frequency)]
 assert rd(0x42def0)==((state.frequency if saved is None else saved)&0xffffffff)
print('PASS1200 original startup prefixes: time→actual srand/rand→frequency default,one draw even with persisted value,exact surviving CRT state')
