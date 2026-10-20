"""Compare recovered option-command transitions with original machine code."""
import ctypes as C, pathlib, struct, random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
ROOT=pathlib.Path(__file__).resolve().parents[2]
p=pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image())
addresses=[0x422278,0x422280,0x42e018,0x42e000,0x42e53c,0x42e144,0x42def4,0x42e004,0x422284]
entries=[0x40c5f1,0x40c606,0x40c661,0x40c676,0x40c68b,0x40c6a0,0x40c6b5,0x40c6ca,0x40c8a3]
commands=[40021,40023,40024,40025,40026,40027,40028,40029,40032]
class Options(C.Structure):_fields_=[('value',C.c_int32*9)]
lib=C.CDLL(str(ROOT/'code/build/liboptions.so'));lib.sniper_options_command.argtypes=[C.POINTER(Options),C.c_uint]
rng=random.Random(87);count=0
for entry,command in zip(entries,commands):
    for _ in range(100):
        values=[rng.choice([0,1,2,-1,2147483647,-2147483648]) for _ in range(9)]
        s=Options((C.c_int32*9)(*values))
        for a,v in zip(addresses,values):u.mem_write(a,struct.pack('<i',v))
        u.emu_start(entry,0x40cddb,count=1000)
        expected=[struct.unpack('<i',u.mem_read(a,4))[0] for a in addresses]
        assert lib.sniper_options_command(C.byref(s),command)==1
        assert list(s.value)==expected,(command,values,list(s.value),expected)
        count+=1
print(f'{count} option-command state transitions match original x86')
