"""Integrated lifecycle: original movement, spawn, reset and CRT RNG execute."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
lib=C.CDLL(str(ROOT/'code/build/libship_world_test.so'))
lib.test_ship_world.argtypes=[C.POINTER(Ship),C.POINTER(Ship),C.POINTER(Ship),C.c_void_p,C.c_void_p,C.POINTER(C.c_int32)]
config=(C.c_int32*72)();lib.sniper_local_config(config)
u.mem_write(0x4281c9,bytes(config))
put(0x4353f4,0x820000);put(0x429f48,0x600000);put(0x429f50,0x700000);put(0x42de7c,32)
put(0x429318,1);put(0x424340,2);put(0x42de80,0);put(0x424324,0);put(0x428a08,0)
put(0x424348,0x830000);put(0x830000,0x830100);put(0x83012c,0x820010)
native_events=[];local_address=0x510000;ms=0
calls={0x4137c0:0,0x414450:0,0x4165f0:0}
def hook(uc,a,size,data):
    if a in calls:calls[a]+=1
    if a not in [0x820000,0x820010,0x407a80]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;value=0
    if a==0x820000:value=ms
    elif a==0x820010:
        native_events[3]+=1;native_events[4]=C.c_int32(get(sp+4)).value
        put(local_address+0x6c,get(sp+4));clean=4
    else:
        native_events[0]+=1;native_events[1]=C.c_int32(get(sp+8)).value;native_events[2]=C.c_int32(get(sp+12)).value
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(87)
grid=bytearray(1024*1024)
for y in range(20,31):
    for x in range(20,31):
        if x in [20,30] or y in [20,30]:grid[y*1024+x]=1
u.mem_write(0x600000,bytes(grid));mapbuf=(C.c_uint8*len(grid)).from_buffer_copy(grid);coverage=(C.c_uint8*8192)()
for case in range(800):
    ship=Ship();local=Ship();parent=Ship()
    for i in range(134):ship.word[i]=rng.randint(-20,60)
    def setf(o,v):ship.word[o//4]=v
    setf(0x40,rng.randrange(5));setf(0x44,rng.choice([1,3]));setf(0x48,rng.randint(350000,450000));setf(0x4c,rng.randint(350000,450000))
    setf(0x50,rng.randint(-6000,6000));setf(0x54,rng.randint(-6000,6000));setf(0x74,1000)
    setf(0x68,rng.choice([7,8]));setf(0x6c,rng.choice([-1,-1,2,3]))
    for o in [0x60,0x64,0x10c]:setf(o,rng.choice([0,0,0,1,50]))
    for o in [0x88,0x8c,0x90,0x94]:setf(o,rng.choice([0,1]))
    setf(0xc4,166);setf(0xc8,1000000);setf(0x5c,600000)
    local.word[0x68//4]=7;local.word[0x6c//4]=2
    for o in [0x48,0x4c,0x50,0x54]:parent.word[o//4]=rng.choice([0,100000,200000])
    local_ref=ship if ship.word[0x44//4]==1 else local
    local_address=0x500000 if local_ref is ship else 0x510000
    ms=10000;dt=rng.choice([0,1,10,25,50]);seed=rng.getrandbits(32);players=rng.choice([1,24,96])
    values=(C.c_int32*11)(ms,dt,C.c_int32(seed).value,players)
    u.mem_write(0x500000,bytes(ship));u.mem_write(0x510000,bytes(local));u.mem_write(0x530000,bytes(parent))
    put(0x4218e0,local_address);put(0x42933c,local_address);put(0x429340,0x530000);put(0x429344,0)
    put(0x422f78,seed);put(0x429328,players);put(0x4292fc,-7);u.mem_write(0x700000,bytes(8192))
    put(0x800000,0x810000);put(0x800004,dt);native_events[:]=[0]*5
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x413fa0,0x810000,count=1000000)
    assert u.reg_read(UC_X86_REG_EIP)==0x810000
    lib.test_ship_world(C.byref(ship),C.byref(local_ref),C.byref(parent),mapbuf,coverage,values)
    expected=bytes(u.mem_read(0x500000,536))
    assert bytes(ship)==expected,(case,[hex(i) for i in range(0,536,4) if bytes(ship)[i:i+4]!=expected[i:i+4]])
    assert bytes(local_ref)==bytes(u.mem_read(local_address,536)),(case,'local')
    assert bytes(coverage)==bytes(u.mem_read(0x700000,8192)),(case,'coverage')
    assert values[4]&0xffffffff==get(0x422f78),(case,'random')
    assert values[5]==C.c_int32(get(0x4292fc)).value,(case,'dirty')
    assert list(values)[6:]==native_events,(case,'events',list(values)[6:],native_events)
assert all(calls.values()),calls
print('800 integrated updates match original ships, map coverage, RNG, spawn flag and external effects;',calls)
