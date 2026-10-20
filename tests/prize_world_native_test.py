"""Sequential prizes and updates with actual original RNG, warp and physics."""
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
lib=C.CDLL(str(ROOT/'code/build/libprize_world_test.so'))
lib.test_prize_begin.argtypes=[C.POINTER(Ship),C.c_void_p,C.c_uint32,Sound,Message]
lib.test_prize_action.argtypes=[C.POINTER(Ship),C.c_int,C.c_int32,C.c_void_p,C.c_void_p]
cevents=[];nevents=[]
@Sound
def sound(_,handle):cevents.append(('sound',handle))
@Message
def message(_,text,duration,color):cevents.append(('message',text.decode(),duration,color))
counts={0x4137c0:0,0x414450:0,0x4165f0:0}
def hook(uc,a,size,data):
    if a in counts:counts[a]+=1
    if a not in [0x820000,0x40aae0,0x407b50]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;value=0
    if a==0x820000:value=10000
    elif a==0x40aae0:
        assert [get(sp+i) for i in [4,8,12,16]]==[0]*4
        nevents.append(('sound',uc.reg_read(UC_X86_REG_ECX)));clean=16
    else:
        text=bytes(u.mem_read(get(sp+4),128)).split(b'\0',1)[0].decode()
        nevents.append(('message',text,get(sp+8),get(sp+12)))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
config=(C.c_int32*72)();lib.sniper_local_config(config)
for a in [0x428289,0x42828d]:config[(a-0x4281c9)//4]=250
u.mem_write(0x4281c9,bytes(config))
for a,v in [(0x42190c,101),(0x4218ec,102),(0x4218e8,103),(0x4353f4,0x820000),
            (0x429f48,0x600000),(0x429f50,0x700000),(0x42de7c,32),(0x4218e0,0x500000),
            (0x429318,1),(0x424340,1),(0x429328,1),(0x428a08,0),(0x42de80,0),(0x424324,0)]:put(a,v)
mapbuf=(C.c_uint8*(1024*1024))();coverage=(C.c_uint8*8192)();results=(C.c_uint32*2)();rng=random.Random(87)
steps=0;expiries=0
for case in range(100):
    ship=Ship()
    for i in range(134):ship.word[i]=rng.randint(0,5)
    for off,v in [(0x40,case%5),(0x44,1),(0x48,8000000),(0x4c,8000000),
      (0x50,0),(0x54,0),(0x5c,600000),(0x60,0),(0x64,0),(0x68,7),(0x6c,-1),
      (0x74,1000),(0xc4,166),(0xc8,1000000),(0xcc,160),(0xe0,10),(0xe4,2000),
      (0x10c,0),(0x108,0)]:ship.word[off//4]=v
    seed=rng.getrandbits(32);u.mem_write(0x500000,bytes(ship));u.mem_write(0x700000,bytes(8192))
    put(0x422f78,seed);put(0x4292fc,-7)
    lib.test_prize_begin(C.byref(ship),mapbuf,seed,sound,message)
    # Consecutive active re-awards must not overwrite saved state or refresh time.
    actions=[(1,17),(1,17),(1,18),(1,18)]+[(0,50)]*5
    actions += [(1,rng.choice([-21,-5,-1,1,5,9,15,20,21])),(1,7),(1,-7),
                (1,17),(0,50),(0,50),(0,50),(0,50),(0,50)]
    for index,(prize,value) in enumerate(actions):
        before=ship.word[0x10c//4]
        put(0x800000,0x810000);put(0x800004,value)
        u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000)
        cevents.clear();nevents.clear()
        u.emu_start(0x415030 if prize else 0x413fa0,0x810000,count=1000000)
        assert u.reg_read(UC_X86_REG_EIP)==0x810000
        lib.test_prize_action(C.byref(ship),prize,value,results,coverage)
        expected=bytes(u.mem_read(0x500000,536))
        diffs=[hex(i) for i in range(0,536,4) if bytes(ship)[i:i+4]!=expected[i:i+4]]
        assert not diffs,(case,index,prize,value,diffs)
        assert results[0]==get(0x422f78),(case,index,'rng')
        assert results[1]==get(0x4292fc),(case,index,'dirty')
        assert bytes(coverage)==bytes(u.mem_read(0x700000,8192)),(case,index,'coverage')
        assert cevents==nevents,(case,index,cevents,nevents)
        if not prize and before>0 and ship.word[0x10c//4]<=0:expiries+=1
        steps+=1
assert expiries==200,expiries
assert all(counts.values()),counts
print(f'{steps} sequential prize/update actions match ships, RNG, coverage and messages; {expiries} superpower expiries; original calls {counts}')
