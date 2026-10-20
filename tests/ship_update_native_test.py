"""Full413fa0 comparison; external spawn/move/network calls are controlled seams."""
import ctypes as C
import pathlib, random, struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
p=pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x300000);u.mem_write(0x400000,p.get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def signed(x):return C.c_int32(x).value
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Lookup=C.CFUNCTYPE(C.c_void_p,C.c_void_p,C.c_int32)
Detach=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
Spawn=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int,C.c_int)
Move=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,C.c_int)
Random=C.CFUNCTYPE(C.c_int32,C.c_void_p)
class World(C.Structure):
    _fields_=[('local_id',C.c_int32),('respawn_delay',C.c_int32),('remote_energy',C.c_int32),
      ('jitter_period',C.c_int32),('jitter_radius',C.c_int32),('ability_cost',C.c_int32*4),
      ('milliseconds',C.c_uint32),('local_team',C.c_int32),('coverage',C.POINTER(C.c_uint8)),
      ('coverage_stride',C.c_int32),('lookup',Lookup),('detach',Detach),('spawn',Spawn),
      ('move',Move),('random',Random),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libship_update.so'))
lib.sniper_ship_update.argtypes=[C.POINTER(Ship),C.POINTER(World),C.c_int32]
coverage=(C.c_uint8*8192)();parent=Ship();events=[];native=[];rindex=0;nindex=0
@Lookup
def lookup(_,pid):return C.addressof(parent) if pid==2 else None
@Detach
def detach(_,pid):
    events.append(('detach',pid));ship.word[0x6c//4]=-1
@Spawn
def spawn(_,s,a,b):
    events.append(('spawn',a,b));s.contents.word[0x48//4]=333;s.contents.word[0x5c//4]=555
@Move
def move(_,s,dt,effects):
    events.append(('move',dt,effects));s.contents.word[0x48//4]+=17;s.contents.word[0x4c//4]-=9
@Random
def rand(_):
    global rindex
    value=random_values[rindex];rindex+=1;events.append(('random',value));return value
w=World(1,150,1000000,50,200,(C.c_int32*4)(100,200,300,400),10000,7,coverage,32,lookup,detach,spawn,move,rand,None)
put(0x4353f4,0x600000);put(0x424348,0x601000);put(0x601000,0x601100);put(0x60112c,0x600030)
put(0x4218e0,0x510000);put(0x429f50,0x520000);put(0x42de7c,32)
for a,v in [(0x429318,1),(0x428219,150),(0x42824d,1000000),(0x4282a1,50),(0x42829d,200),
            (0x428225,100),(0x428221,200),(0x428299,300),(0x428229,400)]:put(a,v)

def hook(uc,a,size,data):
    global nindex
    if a not in [0x600000,0x600030,0x4137c0,0x414450,0x4165f0]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;value=0
    if a==0x600000:value=w.milliseconds
    elif a==0x600030:
        native.append(('detach',signed(get(sp+4))));put(0x50006c,-1);clean=4
    elif a==0x4137c0:
        native.append(('spawn',get(sp+4),get(sp+8)));put(0x500048,333);put(0x50005c,555);clean=8
    elif a==0x414450:
        native.append(('move',signed(get(sp+4)),get(sp+24)))
        assert [get(sp+i) for i in [8,12,16,20]]==[0x500048,0x50004c,0x500050,0x500054]
        put(0x500048,get(0x500048)+17);put(0x50004c,get(0x50004c)-9);clean=24
    else:
        value=random_values[nindex];nindex+=1;native.append(('random',value))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(413)
paths=set()
for case in range(2400):
    ship=Ship();u.mem_write(0x429338,b'\0'*16);put(0x429340,0x530000)
    for i in range(134):ship.word[i]=rng.randint(-2000,2000)
    def field(o,v):ship.word[o//4]=v
    field(0x44,rng.choice([1,3]));field(0x48,rng.choice([-1,0,64000,120000,16383999,17000000]))
    field(0x4c,rng.choice([-64000,64000,120000,17000000]));field(0x68,rng.choice([7,8]))
    field(0x74,1000-rng.choice([0,300,301,500,501,-1]));field(0x6c,rng.choice([-1,2,3]))
    for o in [0x60,0x64,0x104,0x108,0x10c,0xbc,0xb0]:field(o,rng.choice([-1,0,0,0,1,25,51]))
    for o in [0x88,0x8c,0x90,0x94]:field(o,rng.randint(0,1))
    field(0x5c,rng.randint(-1,1000000));field(0xc4,166);field(0xc8,1000000)
    for o in range(0x110,0x154,4):field(o,rng.randint(0,10000))
    for o in [0x48,0x4c,0x50,0x54]:parent.word[o//4]=rng.choice([0,100000,200000])
    # Include negative signed age after system-clock rollover.
    w.milliseconds=rng.choice([0,10000,10000,0xffffffff])
    dt=rng.choice([0,1,10,25,50]);random_values=[rng.randrange(32768) for _ in range(3)]
    rindex=nindex=0;events.clear();native.clear()
    initial=bytes(ship);u.mem_write(0x500000,initial);u.mem_write(0x530000,bytes(parent));put(0x510068,7)
    initial_coverage=rng.randbytes(8192);u.mem_write(0x520000,initial_coverage);C.memmove(coverage,initial_coverage,8192)
    put(0x540000,0x600100);put(0x540004,dt)
    u.reg_write(UC_X86_REG_ESP,0x540000);u.reg_write(UC_X86_REG_ECX,0x500000)
    u.emu_start(0x413fa0,0x600100,count=2000)
    assert u.reg_read(UC_X86_REG_EIP)==0x600100
    lib.sniper_ship_update(C.byref(ship),C.byref(w),dt)
    expected=bytes(u.mem_read(0x500000,536))
    if bytes(ship)!=expected:
        diffs=[hex(i) for i in range(0,536,4) if bytes(ship)[i:i+4]!=expected[i:i+4]]
        raise AssertionError((case,diffs,events,native))
    assert bytes(coverage)==bytes(u.mem_read(0x520000,8192)),case
    assert events==native,(case,events,native)
    paths.update(e[0] for e in events)
assert paths=={'move','spawn','random','detach'},paths
print('2400 full ship-update cases match all536 object bytes, coverage bitmap and ordered external calls')
