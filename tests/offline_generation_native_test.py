"""Full411450 including actual CRT RNG and repeated occupied-cell retries."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
Create=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_int32,C.c_int32)
cevents=[];nevents=[];ci=ni=0;times=[]
@Clock
def clock(_):
    global ci
    value=times[ci];ci+=1;cevents.append(('clock',value));return value
@Create
def create(_,x,y,code,lifetime):cevents.append(('create',x,y,code,lifetime))
def hook(uc,a,size,data):
    global ni
    if a not in [0x820000,0x404f10]:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x820000:
        value=times[ni];ni+=1;nevents.append(('clock',value));uc.reg_write(UC_X86_REG_EAX,value)
    else:nevents.append(('create',*[get(sp+4*i) for i in range(1,5)]))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x820000);put(0x429f48,0x600000)
lib=C.CDLL(str(ROOT/'code/build/liboffline_generation.so'))
lib.sniper_offline_generate.argtypes=[C.POINTER(C.c_uint32),C.POINTER(C.c_uint32),C.c_void_p,Clock,Create,C.c_void_p]
rng=random.Random(411450);count=0;created=0
for density in [0,0.5,0.95]:
    grid=bytearray(1024*1024)
    for y in range(380,644):
        for x in range(380,644):grid[y*1024+x]=rng.choice([1,152,250,255]) if rng.random()<density else 0
    buf=(C.c_uint8*len(grid)).from_buffer_copy(grid);u.mem_write(0x600000,bytes(grid))
    cases=[(1000,t) for t in [9999,10000,14999,15000,15009,15010,20000]]
    cases += [(429496729,0),(0,0xffffffff),(0xffffffff,0)]
    cases += [(rng.randrange(429496730),rng.getrandbits(32)) for _ in range(140)]
    for previous,ms in cases:
        times=[ms,rng.getrandbits(32)];ci=ni=0;cevents.clear();nevents.clear()
        seed=rng.getrandbits(32);put(0x422f78,seed);put(0x500008,previous);put(0x800000,0x810000)
        u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000)
        u.emu_start(0x411450,0x810000,count=2000000)
        assert u.reg_read(UC_X86_REG_EIP)==0x810000
        prev=C.c_uint32(previous);state=C.c_uint32(seed)
        lib.sniper_offline_generate(C.byref(prev),C.byref(state),buf,clock,create,None)
        assert prev.value==get(0x500008)
        assert state.value==get(0x422f78)
        assert cevents==nevents,(density,previous,ms,cevents,nevents)
        count+=1;created+=sum(e[0]=='create' for e in cevents)
print(f'{count} generation calls match clocks, timestamp, RNG and create arguments; {created} generated pickups')
