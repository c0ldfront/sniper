"""Compare bounded numeric corpus at actual startup CW027f; no math stubs.
Hardware evidence remains authoritative if the emulator diverges; this registered
test fails rather than silently accepting such differences.
"""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
SHIP=0x500000;STACK=0x510000
lib=C.CDLL(str(ROOT/'code/build/libflight_math.so'))
lib.sniper_test_flight_numeric_initialize.restype=C.c_ushort
assert lib.sniper_test_flight_numeric_initialize()==0x027f
lib.sniper_flight_accelerate.argtypes=[C.c_int32]*4+[C.POINTER(C.c_int32)]*2
lib.sniper_flight_limit.argtypes=[C.c_int32]+[C.POINTER(C.c_int32)]*2
lib.sniper_flight_accelerate.restype=lib.sniper_flight_limit.restype=None
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def init(vx,vy):
    put(SHIP+0x50,vx);put(SHIP+0x54,vy)
    u.reg_write(UC_X86_REG_ESP,STACK)
    u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    assert u.reg_read(UC_X86_REG_FPCW)==0x027f
put(0x4218e0,SHIP);put(0x4230fc,0)
rng=random.Random(87);count=0
for heading in range(0,40000,100):
    for thrust in (-1,1):
        ticks=rng.randrange(51);acc=rng.randrange(1,200)
        vx=rng.randrange(-5000,5001);vy=rng.randrange(-5000,5001)
        init(vx,vy);put(SHIP+0x58,heading);put(0x4218c8,ticks)
        u.reg_write(UC_X86_REG_EBX,acc);u.reg_write(UC_X86_REG_ESI,thrust&0xffffffff)
        u.emu_start(0x4061d5,0x406235,count=500)
        x=C.c_int32(vx);y=C.c_int32(vy)
        lib.sniper_flight_accelerate(heading,thrust,acc,ticks,C.byref(x),C.byref(y))
        assert (x.value,y.value)==(get(SHIP+0x50),get(SHIP+0x54)),('acc',heading,thrust,acc,ticks,x.value,y.value,get(SHIP+0x50),get(SHIP+0x54))
        count+=1
print(f'{count} acceleration cases match original x87 and CRT conversion')
cases=[(vx,vy,limit) for vx,vy in [(0,0),(0,1),(1,0),(0,-1),(-1,0),(1,1),(-1,-1),(3000,4000)] for limit in (0,1,10,1000,5000)]
cases += [(rng.randrange(-20000,20001),rng.randrange(-20000,20001),rng.randrange(10001)) for _ in range(2000)]
differences=[]
for vx,vy,limit in cases:
    init(vx,vy);u.reg_write(UC_X86_REG_EDI,limit)
    u.emu_start(0x4062a6,0x406332,count=500)
    x=C.c_int32(vx);y=C.c_int32(vy)
    lib.sniper_flight_limit(limit,C.byref(x),C.byref(y))
    if (x.value,y.value)!=(get(SHIP+0x50),get(SHIP+0x54)):
        differences.append((vx,vy,limit,x.value,y.value,get(SHIP+0x50),get(SHIP+0x54)))
print(f'CW027f speed-limit comparison: {len(differences)} differences in {len(cases)} cases; hardware test is authoritative')
if differences: print('First difference (vx,vy,limit,recovered x/y,Unicorn x/y):', differences[0])

assert not differences, ("Investigate numeric regression or emulator discrepancy against hardware evidence", differences[:4])
