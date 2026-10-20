"""Whole-object comparison of original exhaust and second effect-ring emitter."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(ROOT/'code/build/libship_effect.so'))
rng=random.Random(87);count=0
for entry,name in [(0x415ac0,'sniper_ship_exhaust'),(0x415b80,'sniper_ship_effect1')]:
    fn=getattr(lib,name);fn.argtypes=[C.POINTER(C.c_int32)];fn.restype=None
    for heading in range(0,40000,125):
        values=[rng.randrange(-2**31,2**31) for _ in range(0x218//4)]
        values[0x48//4]=rng.randrange(-10000,16384000);values[0x4c//4]=rng.randrange(-10000,16384000)
        values[0x58//4]=heading;values[0x214//4]=count%12
        state=(C.c_int32*len(values))(*values)
        u.mem_write(0x500000,bytes(state));u.mem_write(0x510000,struct.pack('<I',0x520000))
        u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x510000)
        u.reg_write(UC_X86_REG_FPCW,0x37f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
        u.emu_start(entry,0x520000,count=1000)
        fn(state)
        assert bytes(state)==bytes(u.mem_read(0x500000,0x218)),(name,heading)
        count+=1
print(f'{count} effect emissions match whole original object including ring wrap')
