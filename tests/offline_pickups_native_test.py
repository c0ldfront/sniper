"""Full update through offline pickup backend, collection and prize effects."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
Message=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32)
lib=C.CDLL(str(ROOT/'code/build/liboffline_pickups_test.so'))
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
lib.test_offline_pickups.argtypes=[C.POINTER(Ship),C.POINTER(Pickup),C.c_int32,C.c_void_p,C.c_int32,C.c_uint32,Sound,Message,C.c_void_p,C.c_void_p]
cevents=[];nevents=[]
@Sound
def sound(_,handle):cevents.append(('sound',handle))
@Message
def message(_,text,duration,color):cevents.append(('message',text.decode(),duration,color))
counts={0x411560:0,0x404910:0,0x415030:0,0x4137c0:0}
freed=0
def hook(uc,a,size,data):
    global freed
    if a in counts:counts[a]+=1
    if a not in [0x820000,0x40aae0,0x407b50,0x416620]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;value=0
    if a==0x820000:value=10000
    elif a==0x416620:freed=(freed*31+get(get(sp+4))+1)&0xffffffff
    elif a==0x40aae0:
        assert [get(sp+i) for i in [4,8,12,16]]==[0]*4
        nevents.append(('sound',uc.reg_read(UC_X86_REG_ECX)));clean=16
    else:
        text=bytes(u.mem_read(get(sp+4),128)).split(b'\0',1)[0].decode()
        nevents.append(('message',text,get(sp+8),get(sp+12)))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
config=(C.c_int32*72)();lib.sniper_local_config(config)
u.mem_write(0x4281c9,bytes(config))
for a,v in [(0x42190c,101),(0x4218ec,102),(0x4218e8,103),(0x4353f4,0x820000),
            (0x429f48,0x600000),(0x429f50,0x700000),(0x42de7c,32),(0x4218e0,0x500000),
            (0x429318,0),(0x424340,0),(0x429328,1),(0x428a08,0),(0x42de80,0),(0x424324,0)]:put(a,v)

put(0x424348,0x540000);put(0x540000,0x420008)
mapbuf=(C.c_uint8*(1024*1024))();coverage=(C.c_uint8*8192)();results=(C.c_uint32*36)();rng=random.Random(411560)
for case in range(252):
    ship=Ship()
    for off,v in [(0x40,case%5),(0x44,0),(0x48,400000),(0x4c,400000),
      (0x50,100),(0x54,100),(0x5c,600000),(0x60,0),(0x64,0),(0x68,7),(0x6c,-1),
      (0x74,1000),(0xc4,166),(0xc8,1000000),(0xcc,160),(0xe0,10),(0xe4,2000),
      (0x10c,0),(0x108,0),(0xd0,1)]:ship.word[off//4]=v
    # Two neighbouring records plus duplicate coordinate and an untouched distant item.
    code=case%21+1
    if case>=126:code=-code
    objects=(Pickup*4)(Pickup(0,25,24,100,code),Pickup(1,25,25,100,code),
        Pickup(2,25,25,100,code),Pickup(3,800,800,100,1))
    C.memset(mapbuf,0,len(mapbuf))
    for i,p in enumerate(objects):
        mapbuf[p.y*1024+p.x]=255
        u.mem_write(0x530000+i*32,bytes(p));put(0x429288+i*4,0x530000+i*32)
    u.mem_write(0x600000,bytes(mapbuf));u.mem_write(0x500000,bytes(ship));u.mem_write(0x700000,bytes(8192))
    seed=rng.getrandbits(32);put(0x422f78,seed);put(0x4292fc,-7);put(0x428a08,4)
    ticks=[1,5,10][case%3];put(0x800000,0x810000);put(0x800004,ticks)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000)
    cevents.clear();nevents.clear();freed=0
    u.emu_start(0x413fa0,0x810000,count=1000000)
    assert u.reg_read(UC_X86_REG_EIP)==0x810000
    lib.test_offline_pickups(C.byref(ship),objects,4,mapbuf,ticks,seed,sound,message,results,coverage)
    expected=bytes(u.mem_read(0x500000,536))
    diffs=[hex(i) for i in range(0,536,4) if bytes(ship)[i:i+4]!=expected[i:i+4]]
    assert not diffs,(case,code,diffs)
    assert results[0]==get(0x422f78),(case,'rng')
    assert results[1]==get(0x4292fc),(case,'dirty')
    assert results[2]==get(0x428a08),(case,'count')
    assert results[3]==freed,(case,'release order')
    assert list(results)[4:4+results[2]]==[get(get(0x429288+i*4)) for i in range(results[2])]
    assert bytes(coverage)==bytes(u.mem_read(0x700000,8192)),(case,'coverage')
    assert bytes(mapbuf)==bytes(u.mem_read(0x600000,len(mapbuf))),(case,'map')
    assert cevents==nevents,(case,cevents,nevents)
assert all(counts.values()),counts
print(f'252 full offline pickup updates match ship/map/list/RNG/coverage and messages; original calls {counts}')
