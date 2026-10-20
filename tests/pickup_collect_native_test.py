"""Full404910 and actual destructor412ae0/list compaction, award/free seams."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
Lookup=C.CFUNCTYPE(C.c_void_p,C.c_void_p,C.c_int32)
Release=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Pickup))
Award=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32)
class World(C.Structure):
    _fields_=[('items',C.POINTER(C.POINTER(Pickup))),('count',C.c_int32),('map',C.POINTER(C.c_uint8)),
      ('local',C.POINTER(Ship)),('sharing_enabled',C.c_int32),('sharing_limit',C.c_int32),
      ('lookup',Lookup),('release',Release),('award',Award),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libpickup_collect.so'));lib.sniper_pickup_collect.argtypes=[C.POINTER(World)]+[C.c_int32]*4
local=Ship();other=Ship();grid=(C.c_uint8*(1024*1024))();cevents=[];nevents=[];present=True
@Lookup
def lookup(_,pid):return C.addressof(other) if present and pid==3 else None
@Release
def release(_,p):
    v=p.contents;cevents.append(('release',v.age,w.count,grid[v.y*1024+v.x]))
@Award
def award(_,s,code):
    cevents.append(('award',code));s.contents.word[0x5c//4]=code
w=World(None,0,grid,C.pointer(local),1,50,lookup,release,award,None)
def hook(uc,a,size,data):
    if a not in [0x416620,0x415030]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x416620:
        p=get(sp+4);x,y=get(p+4),get(p+8)
        nevents.append(('release',get(p),get(0x428a08),u.mem_read(0x600000+y*1024+x,1)[0]))
    else:
        assert uc.reg_read(UC_X86_REG_ECX)==0x500000
        code=get(sp+4);nevents.append(('award',code));put(0x50005c,code);clean=4
    uc.reg_write(UC_X86_REG_EIP,get(sp)&0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook);put(0x429f48,0x600000);put(0x4218e0,0x500000)
rng=random.Random(404910);count=0;awards=0;releases=0
codes=[0,1,-1,7,-7,17,-17,18,-18,14,-14,6,-6,0x7fffffff,-0x80000000]
# Explicit sharing gate boundaries combined with varied pickup list shapes.
for code in codes:
    for scenario in range(96):
        C.memset(C.byref(local),0,C.sizeof(local));C.memset(C.byref(other),0,C.sizeof(other));C.memset(grid,0,len(grid))
        local.word[0x44//4]=1;local.word[0x6c//4]=2;local.word[0x68//4]=7
        other.word[0x68//4]=7 if scenario%4 else 8
        local.word[0xa0//4]=[49,50,51,-1][scenario%4]
        w.sharing_enabled=0 if scenario%8==0 else 1;w.sharing_limit=50
        present=scenario%8!=1;collector=[1,2,3,3][scenario%4]
        # Independent targeted shared-path scenarios in upper half.
        if scenario>=32:
            collector=3;other.word[0x68//4]=7;local.word[0xa0//4]=49
            present=True;w.sharing_enabled=1
        if scenario>=64:
            gate=(scenario-64)%8
            if gate==0:w.sharing_enabled=0
            elif gate==1:local.word[0xa0//4]=50
            elif gate==2:local.word[0xa0//4]=51
            elif gate==3:present=False
            elif gate==4:other.word[0x68//4]=8
            elif gate==5:local.word[0xa0//4]=-1
            elif gate==6:collector=1
            elif gate==7:collector=2
        position=[(5*16000,6*16000),(-1,-1),(-16000,16000),(16384000,16000)][scenario%4]
        local.word[0x48//4],local.word[0x4c//4]=position
        for y,x in [(6,5),(0,0)]:grid[y*1024+x]=152 if scenario%2==0 else 1
        # Includes no matches, duplicates and empty list; coordinates may coincide
        # with safe tile to verify removal happens before award eligibility.
        target=(5,6) if scenario%3==0 else (10,11)
        objects=[]
        for i in range(scenario%7):
            xy=target if rng.randrange(3)!=0 else (20+i,30+i)
            objects.append(Pickup(i,*xy,100,7));grid[xy[1]*1024+xy[0]]=255
        pointers=(C.POINTER(Pickup)*len(objects))(*(C.pointer(p) for p in objects));w.items=pointers;w.count=len(objects)
        for i,p in enumerate(objects):
            addr=0x530000+i*32;u.mem_write(addr,bytes(p));put(0x429288+4*i,addr)
        u.mem_write(0x500000,bytes(local));u.mem_write(0x510000,bytes(other));u.mem_write(0x600000,bytes(grid))
        for a,v in [(0x428a08,len(objects)),(0x4282ad,w.sharing_enabled),(0x4282b9,50),(0x429344,0x510000 if present else 0)]:put(a,v)
        for i,v in enumerate([0x810000,*target,collector,code]):put(0x800000+4*i,v)
        cevents.clear();nevents.clear();u.reg_write(UC_X86_REG_ESP,0x800000)
        u.emu_start(0x404910,0x810000,count=100000)
        assert u.reg_read(UC_X86_REG_EIP)==0x810000
        lib.sniper_pickup_collect(C.byref(w),*target,collector,code)
        assert cevents==nevents,(code,scenario,cevents,nevents)
        assert bytes(local)==bytes(u.mem_read(0x500000,536)),(code,scenario,'ship')
        assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),(code,scenario,'map')
        assert w.count==get(0x428a08)
        assert [pointers[i].contents.age for i in range(len(objects))]==[get(get(0x429288+4*i)) for i in range(len(objects))]
        count+=1;awards+=sum(e[0]=='award' for e in cevents);releases+=sum(e[0]=='release' for e in cevents)
assert awards and releases
print(f'{count} pickup removal/routing cases match list, map, ship and call order; {releases} releases, {awards} awards')
