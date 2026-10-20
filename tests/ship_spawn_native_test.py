"""Compare whole-object changes from original reset block, including untouched fields."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Defaults(C.Structure):_fields_=[(n,C.c_int32) for n in ['recharge','energy_maximum','rotation','acceleration','speed','rotation_bonus','acceleration_bonus','speed_bonus','energy_bonus','recharge_bonus']]
lib=C.CDLL(str(ROOT/'code/build/libship_spawn.so'));lib.sniper_ship_reset.argtypes=[C.POINTER(C.c_int32),C.POINTER(Defaults)];lib.sniper_ship_reset.restype=None
addresses=[0x428251,0x42824d,0x428241,0x428245,0x428249,0x428255,0x428259,0x42825d,0x428261,0x428265]

lib.sniper_ship_spawn.argtypes=[C.POINTER(C.c_int32),C.POINTER(Defaults),C.POINTER(C.c_uint8),C.c_int32,C.c_uint32,C.POINTER(C.c_uint32),C.c_int,C.c_int,C.POINTER(C.c_int32)]
lib.sniper_ship_spawn.restype=None
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
clock=0
def hook(uc,addr,size,data):
    if addr!=0x820000:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    uc.reg_write(UC_X86_REG_EAX,clock)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
put(0x4353f4,0x820000);put(0x429f48,0x600000)
rng=random.Random(87);count=0
# Rejection-heavy map, plus clear map. Every nonzero byte blocks spawning.
for density in [0,0.15]:
    grid=bytes(rng.choice([1,152,250,255]) if rng.random()<density else 0 for _ in range(1024*1024))
    u.mem_write(0x600000,grid);c_grid=(C.c_uint8*len(grid)).from_buffer_copy(grid)
    for i in range(160):
        values=[rng.randrange(-2**31,2**31) for _ in range(0x110//4)]
        values[0x40//4]=rng.randrange(5)
        params=[rng.randrange(1,200000) for _ in addresses]
        state=(C.c_int32*len(values))(*values);d=Defaults(*params)
        seed=rng.getrandbits(32);clock=rng.getrandbits(32)
        players=rng.choice([0,1,24,71,72,96,1024]);reset=i%2;move=(i//2)%2
        dirty=C.c_int32(-7);random_state=C.c_uint32(seed)
        u.mem_write(0x500000,bytes(state))
        for a,v in zip(addresses,params):put(a,v)
        for a,v in [(0x422f78,seed),(0x429328,players),(0x4292fc,-7),(0x800000,0x810000),(0x800004,reset),(0x800008,move)]:put(a,v)
        u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000)
        u.emu_start(0x4137c0,0x810000,count=2000000)
        assert u.reg_read(UC_X86_REG_EIP)==0x810000
        lib.sniper_ship_spawn(state,C.byref(d),c_grid,players,clock,C.byref(random_state),reset,move,C.byref(dirty))
        assert bytes(state)==bytes(u.mem_read(0x500000,len(bytes(state)))),(density,i)
        assert random_state.value==get(0x422f78),(density,i,'rng')
        assert dirty.value==C.c_int32(get(0x4292fc)).value
        count+=1
print(f'{count} full spawn/reset calls match original object, RNG state and dirty flag')
