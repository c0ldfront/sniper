"""Compare ordered draw arguments from original full particle renderer."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(ROOT/'code/build/libship_render.so'))
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int32,C.c_int32,C.c_int32)
Label=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32,C.c_int)
lib.sniper_ship_render.argtypes=[C.POINTER(C.c_int32)]*3+[C.POINTER(C.c_uint32),C.c_int,C.c_int]+[C.c_int32]*5+[Draw,Draw,Label,C.c_void_p]
lib.sniper_ship_render.restype=None
native=[];recovered=[]
@Draw
def draw(_,resource,x,y,frame):recovered.append(('sprite',resource,x,y,frame))
@Label
def label(_,text,x,y,color):recovered.append(('text',text,x,y,color))
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def hook(uc,addr,size,data):
    if addr==0x415d60:
        sp=uc.reg_read(UC_X86_REG_ESP)
        text=bytes(uc.mem_read(get(sp+4),128)).split(b'\0')[0]
        native.append(('text',text,*[C.c_int32(get(sp+i)).value for i in (8,12,16)]))
        uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4);return
    if addr!=0x4105c0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    assert get(sp+16)==1 and get(sp+20)==0
    native.append(('sprite',uc.reg_read(UC_X86_REG_ECX),*[C.c_int32(get(sp+i)).value for i in (4,8,12)]))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+24)
u.hook_add(UC_HOOK_CODE,hook)

for slot,addr in enumerate([0x428308,0x42830c,0x428310,0x428314,0x428318,0x429320,0x429f60,0x429304,0x428330]):put(addr,slot)
put(0x42817c,173);put(0x428180,147)
put(0x429318,7);put(0x4218e0,0x530000)
rng=random.Random(87)
for i in range(1000):
    values=[rng.randrange(-100000,100001) for _ in range(0x218//4)]
    for off,v in [(0x44,rng.choice([7,8])),(0x40,rng.randrange(4)),(0x60,rng.choice([0,0,1,51,102])),(0x6c,rng.choice([-1,-1,3])),(0xbc,rng.choice([-1,0,1,24,25,48])),(0x84,rng.choice([0,1]))]:values[off//4]=v
    values[0xa4//4]=rng.choice([0,1,2,3])
    if i%7==0: values[0x48//4]=-1
    for slot in range(12):
        values[0x15c//4+slot*4]=rng.choice([-1,0,1,25,52,57])
        values[0x160//4+slot*4]=rng.choice([0,1])
    blink=C.c_uint32(rng.choice([0,1,2,3,4,0xffffffff]));detail=rng.randrange(2)
    put(0x433850,blink.value);put(0x422280,detail)
    values[0x6c//4]=rng.choice([-1,3,7])
    values[0x5c//4]=rng.choice([0,250000,250001,900000]);values[0xc8//4]=1000000
    values[0xa0//4]=rng.randrange(1000);values[0x68//4]=rng.randrange(2)
    state=(C.c_int32*len(values))(*values);cx=rng.randrange(-10000,10000);cy=rng.randrange(-10000,10000);field94=rng.randrange(2)
    C.memset(C.addressof(state),0,64);C.memmove(C.addressof(state),b'Pilot',5);C.memmove(C.addressof(state)+32,b'Squad',5)
    local=(C.c_int32*len(values))();parent=(C.c_int32*len(values))()
    local[0x44//4]=7;local[0x94//4]=field94;local[0xa0//4]=500;local[0x68//4]=1
    parent[0xc0//4]=rng.randrange(10);mode=rng.randrange(4)
    # Local object aliases target for local-player cases, matching actual runtime.
    local_ptr=state if values[0x44//4]==7 else local
    local_addr=0x500000 if values[0x44//4]==7 else 0x530000
    if local_addr==0x500000:state[0x94//4]=field94
    for a,v in [(0x4218e0,local_addr),(0x429338+12,0x540000),(0x428344,mode),(0x4282bd,50),(0x4282c1,35),(0x4282c5,10)]:put(a,v)
    u.mem_write(0x500000,bytes(state));u.mem_write(0x530000,bytes(local));u.mem_write(0x540000,bytes(parent))
    put(0x510000,0x520000);put(0x510004,cx);put(0x510008,cy)
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x510000)
    native.clear();recovered.clear();u.emu_start(0x413cb0,0x520000,count=100000)
    lib.sniper_ship_render(state,local_ptr,parent,C.byref(blink),detail,mode,50,35,10,cx,cy,draw,draw,label,None)
    assert u.reg_read(UC_X86_REG_EIP)==0x520000
    assert bytes(parent)==bytes(u.mem_read(0x540000,0x218))
    assert blink.value==get(0x433850)
    assert native==recovered,(i,native,recovered)
    assert bytes(state)==bytes(u.mem_read(0x500000,0x218))
print('1000 full ship-render calls match original interleaved sprite/text calls, object and parent mutations, and blink state')
