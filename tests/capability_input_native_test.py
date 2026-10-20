"""Keyboard table and capability branches against actual x86, including dirty flag."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
lib=C.CDLL(str(root/'code/build/libcapability_input.so'))
lib.sniper_capability_input.argtypes=[C.POINTER(Ship),C.c_int32,C.c_int32,C.POINTER(C.c_int32)]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
shift=0;calls=[]
def hook(uc,a,size,user):
    if a!=0x530000:return
    sp=uc.reg_read(UC_X86_REG_ESP);assert get(sp+4)==16
    calls.append(16);uc.reg_write(UC_X86_REG_EAX,shift&0xffffffff)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(2400):
    ship=Ship()
    for i in range(134):ship.word[i]=r.randrange(-2147483648,2147483648)
    for offset in [0xec,0xf0,0xf4,0xf8,0xfc,0x88,0x8c,0x90,0x94,0x98]:
        ship.word[offset//4]=r.choice([0,0,1,2,-1,-2147483648])
    ship.word[0x5c//4]=r.choice([-1,0,500000,1000000])
    key=r.choice([0x23,0x24,0x2e]);shift=r.choice([0,1,0x8000,-32768,-32767]);dirty=C.c_int32(r.choice([0,1,-1]))
    put(0x4218e0,0x500000);u.mem_write(0x500000,bytes(ship));put(0x4292fc,dirty.value);put(0x422248,key)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EBP,0x530000);calls.clear()
    u.emu_start(0x40549a,0x405d56,count=1000)
    assert lib.sniper_capability_input(C.byref(ship),key,shift,C.byref(dirty))==1
    assert bytes(ship)==bytes(u.mem_read(0x500000,C.sizeof(ship))),case
    assert dirty.value&0xffffffff==get(0x4292fc),case
    assert calls==([] if key==0x2e else [16]),case
print('PASS2400 original keyboard-table/capability cases: full ship, availability, Shift, dirty flag, dead/zero energy')
