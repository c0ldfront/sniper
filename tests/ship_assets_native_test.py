"""Verify asset descriptors against original constructor call arguments."""
import ctypes as C
import pathlib,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(ROOT/'code/build/libship_assets.so'))
class Sprite(C.Structure):_fields_=[(n,C.c_int32) for n in ['resource','origin_x','origin_y','width','height','rows','columns','period','frames']]
ships=(Sprite*9).in_dll(lib,'sniper_ship_assets');effects=(Sprite*2).in_dll(lib,'sniper_effect_assets')
seen=[]
def hook(uc,a,size,data):
    if a!=0x4102b0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    seen.append(struct.unpack('<9i',uc.mem_read(sp+8,36)))
    uc.emu_stop()
u.hook_add(UC_HOOK_CODE,hook)
cases=[(0x403302,ships[i],i*128) for i in range(5)]
cases += [(a,s,0) for a,s in zip([0x40316d,0x40325d,0x403080,0x4030cf],ships[5:])]
cases += [(0x403031,effects[0],0),(0x4032ac,effects[1],0)]
for entry,s,y in cases:
    u.reg_write(UC_X86_REG_ESP,0x500000);u.reg_write(UC_X86_REG_EBX,y)
    seen.clear();u.emu_start(entry,0x4102b1,count=50)
    expected=tuple(getattr(s,n) for n,_ in Sprite._fields_)
    assert seen==[expected],(hex(entry),seen,expected)
print('All11 ship/effect atlas descriptors match original sprite-constructor arguments')
