#!/usr/bin/env python3
from pathlib import Path
import ctypes,struct,random,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=Path(__file__).resolve().parents[2];lib=ctypes.CDLL(str(root/'code/build/libidle_bitmap.so'));fn=lib.sniper87_idle_palette_entries;fn.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
p=pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE'));u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x700000,0x30000)
pk=lambda *v:struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]);rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
counts={'GlobalAlloc':2,'GlobalLock':1,'GlobalUnlock':1,'GlobalFree':1,'CreatePalette':1,'FindResourceA':3,'LoadResource':2,'LockResource':1,'GetDC':1,'SelectPalette':3,'RealizePalette':1,'CreateDIBitmap':6,'ReleaseDC':2,'FreeResource':1};imports={}
for dll in p.DIRECTORY_ENTRY_IMPORT:
 for imp in dll.imports:
  name=imp.name.decode() if imp.name else ''
  if name in counts:
   a=0x720000+len(imports)*16;imports[a]=name;u.mem_write(imp.address,pk(a))
record=[];palette=None

def hook(u,a,n,d):
 global palette
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x72ff00:u.emu_stop();return
 if a not in imports:return
 name=imports[a];v=[rd(sp+4+i*4) for i in range(counts[name])];r=1;record.append((name,v))
 if name in ['GlobalAlloc','GlobalLock']:r=0x710000
 elif name=='CreatePalette':palette=bytes(u.mem_read(v[0],4+struct.unpack('<H',u.mem_read(v[0]+2,2))[0]*4));r=777
 elif name=='LockResource':r=0x700000
 elif name=='GetDC':r=888
 elif name=='CreateDIBitmap':r=999
 u.reg_write(UC_X86_REG_EAX,r);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4+len(v)*4)
u.hook_add(UC_HOOK_CODE,hook);rng=random.Random(412410)
for case in range(330):
 bits=case%33;dib=bytearray(rng.randbytes(1200));struct.pack_into('<I',dib,0,40);struct.pack_into('<H',dib,14,bits);struct.pack_into('<I',dib,32,case%4)
 u.mem_write(0x700000,bytes(dib));sp=0x72e000;u.mem_write(sp,pk(0x72ff00,0x700000,0x701000));u.reg_write(UC_X86_REG_ESP,sp);palette=None;record.clear();u.emu_start(0x4124b0,0x72ff00,count=10000)
 out=ctypes.create_string_buffer(1024);num=ctypes.c_uint32();assert fn(bytes(dib),len(dib),out,ctypes.byref(num))==1;assert num.value==rd(0x701000)
 assert palette==(struct.pack('<HH',0x300,num.value)+out.raw[:num.value*4] if num.value else None)
 # Full original loader exercises nested palette routine and exact pixel offset.
 record.clear();u.mem_write(sp,pk(0x72ff00,123,151,0x701004));u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(0x412410,0x72ff00,count=10000)
 calls=[v for n,v in record if n=='CreateDIBitmap'];assert calls==[[888,0x700000,4,0x700000+40+num.value*4,0x700000,0]]
 assert rd(0x701004)==(777 if num.value else 0)
 assert [v for n,v in record if n=='SelectPalette']==([[888,777,0]] if num.value else [])
 assert [v for n,v in record if n=='RealizePalette']==([[888]] if num.value else [])
print('PASS330 native palette comparisons and330 full original loader traces: all bit depths0..32,RGB order,flags0,clrUsed ignored,pixel offsets,selection/realization')
