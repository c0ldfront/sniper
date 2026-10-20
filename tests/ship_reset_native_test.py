"""Compare whole-object changes from original reset block, including untouched fields."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EDI,UC_X86_REG_EDX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Defaults(C.Structure):_fields_=[(n,C.c_int32) for n in ['recharge','energy_maximum','rotation','acceleration','speed','rotation_bonus','acceleration_bonus','speed_bonus','energy_bonus','recharge_bonus']]
lib=C.CDLL(str(ROOT/'code/build/libship_reset.so'));lib.sniper_ship_reset.argtypes=[C.POINTER(C.c_int32),C.POINTER(Defaults)];lib.sniper_ship_reset.restype=None
addresses=[0x428251,0x42824d,0x428241,0x428245,0x428249,0x428255,0x428259,0x42825d,0x428261,0x428265]
rng=random.Random(87);count=0
for type_ in [-1,0,1,2,3,4,2147483647]:
    for i in range(300):
        values=[rng.randrange(-2**31,2**31) for _ in range(0x110//4)]
        values[0x40//4]=type_
        params=[rng.randrange(-10000,200000) for _ in addresses]
        state=(C.c_int32*len(values))(*values);d=Defaults(*params)
        u.mem_write(0x500000,bytes(state))
        for a,v in zip(addresses,params):u.mem_write(a,struct.pack('<i',v))
        u.reg_write(UC_X86_REG_EDI,0x500000);u.reg_write(UC_X86_REG_EDX,0)
        u.emu_start(0x413a91,0x413bfc,count=200)
        lib.sniper_ship_reset(state,C.byref(d))
        assert bytes(state)==bytes(u.mem_read(0x500000,len(bytes(state)))),(type_,params)
        count+=1
print(f'{count} ship reset cases match whole original object state')
