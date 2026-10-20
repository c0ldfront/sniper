"""Original PageUp/Down table and branches, including integer boundaries."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(root/'code/build/liblist_navigation.so'))
lib.sniper_list_navigation.argtypes=[C.c_int32]*4+[C.POINTER(C.c_int32)]*3
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
shift=0;calls=0
def hook(uc,a,size,user):
    global calls
    if a!=0x530000:return
    sp=uc.reg_read(UC_X86_REG_ESP);assert get(sp+4)==16;calls+=1
    uc.reg_write(UC_X86_REG_EAX,shift&0xffffffff);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87);boundaries=[-2147483648,-2147483647,-24,-1,0,1,2,23,24,480,1024,2147483646,2147483647]
for case in range(2400):
    key=r.choice([0x21,0x22]);shift=r.choice([0,1,0x8000,-32768]);count=r.choice(boundaries);height=r.choice(boundaries)
    selected=C.c_int32(r.choice(boundaries));history=C.c_int32(r.choice(boundaries));dirty=C.c_int32(r.choice(boundaries))
    for a,v in [(0x422248,key),(0x429328,count),(0x42434c,height),(0x429f58,selected.value),(0x42e538,history.value),(0x4289f4,dirty.value)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EBP,0x530000);calls=0
    u.emu_start(0x40549a,0x405d56,count=1000)
    assert lib.sniper_list_navigation(key,shift,count,height,C.byref(selected),C.byref(history),C.byref(dirty))==1
    assert [selected.value,history.value,dirty.value]==[get(a) for a in [0x429f58,0x42e538,0x4289f4]],case
    assert calls==1
print('PASS2400 original PageUp/Down cases: selection/history/dirty, queued Shift, empty lists and signed wrap/bounds')
