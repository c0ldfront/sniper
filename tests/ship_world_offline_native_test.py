"""Full two-pass frame loop with four ships and real lifecycle/physics."""
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
lib.test_ship_world_all.argtypes=[C.POINTER(Ship),C.POINTER(C.c_int32),C.c_void_p,C.c_void_p,C.POINTER(C.c_int32)]
config=(C.c_int32*72)();lib.sniper_local_config(config)
u.mem_write(0x4281c9,bytes(config))
put(0x4353f4,0x820000);put(0x429f48,0x600000);put(0x429f50,0x700000);put(0x42de7c,32)
put(0x429318,0);put(0x424340,2);put(0x42de80,0);put(0x424324,0);put(0x428a08,0)
put(0x424348,0x830000);put(0x830000,0x420008);put(0x83012c,0x820010)
native_events=[];local_address=0x500000;ms=0
calls={0x411620:0,0x4137c0:0,0x414450:0,0x4165f0:0}
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

rng=random.Random(402237);grid=bytearray(1024*1024)
for y in range(20,31):
    for x in range(20,31):
        if x in [20,30] or y in [20,30]:grid[y*1024+x]=1
u.mem_write(0x600000,bytes(grid));mapbuf=(C.c_uint8*len(grid)).from_buffer_copy(grid);coverage=(C.c_uint8*8192)()
count=0;detaches=0
for case in range(50):
    ships=(Ship*4)()
    for i in range(4):
        for j in range(134):ships[i].word[j]=rng.randint(-20,60)
        def field(o,v):ships[i].word[o//4]=v
        for o,v in [(0x40,i),(0x44,i),(0x48,rng.randint(350000,450000)),(0x4c,rng.randint(350000,450000)),
          (0x50,rng.randint(-4000,4000)),(0x54,rng.randint(-4000,4000)),(0x74,1000),
          (0x68,rng.choice([7,8])),(0x6c,rng.choice([-1,-1,0,1,2,9])),(0x5c,600000),(0xc4,166),(0xc8,1000000)]:field(o,v)
        for o in [0x60,0x64,0x10c]:field(o,rng.choice([0,0,0,1,50]))
        for o in [0x88,0x8c,0x90,0x94]:field(o,rng.choice([0,1]))
    if case%3==0:
        # Explicit parent -> child -> grandchild chain,with shuffled traversal.
        for i,a in enumerate([-1,0,1,2]):ships[i].word[0x6c//4]=a
    order=list(range(4));rng.shuffle(order);c_order=(C.c_int32*4)(*order)
    seed=rng.getrandbits(32);dirty=-7
    for frame in range(4):
        ms=10000+frame*500;dt=rng.choice([0,1,10,25,50])
        values=(C.c_int32*11)(ms,dt,C.c_int32(seed).value,0,0,dirty)
        for i in range(4):
            u.mem_write(0x500000+i*536,bytes(ships[i]));put(0x429338+4*i,0x500000+i*536)
        for i,j in enumerate(order):put(0x4283e8+i*4,0x500000+j*536)
        put(0x429338+4*9,0);put(0x4218e0,0x500000);put(0x429328,4)
        put(0x422f78,seed);put(0x4292fc,dirty);put(0x4218c8,dt);u.mem_write(0x700000,bytes(8192))
        native_events[:]=[0]*5;u.reg_write(UC_X86_REG_ESP,0x800000)
        u.emu_start(0x402237,0x40229b,count=2000000)
        assert u.reg_read(UC_X86_REG_EIP)==0x40229b
        lib.test_ship_world_all(ships,c_order,mapbuf,coverage,values)
        assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),(case,frame,'ships')
        assert values[4]&0xffffffff==get(0x422f78),(case,frame,'rng')
        assert values[5]==C.c_int32(get(0x4292fc)).value
        assert list(values)[6:]==native_events,(case,frame,'effects')
        assert bytes(coverage)==bytes(u.mem_read(0x700000,8192)),(case,frame,'coverage')
        seed=values[4]&0xffffffff;dirty=values[5];count+=1;detaches+=native_events[3]
assert all(calls.values())
print(f'{count} offline four-ship frame passes match all objects,RNG,coverage and effects; {calls[0x411620]} actual offline detach calls; original calls {calls}')
