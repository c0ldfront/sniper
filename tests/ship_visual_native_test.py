"""Compare ordered draw arguments from original full particle renderer."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(ROOT/'code/build/libship_visual.so'))
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int32,C.c_int32,C.c_int32)
lib.sniper_ship_visual.argtypes=[C.POINTER(C.c_int32),C.POINTER(C.c_uint32),C.c_int,C.c_int32,C.c_int,C.c_int32,C.c_int32,Draw,Draw,C.c_void_p]
lib.sniper_ship_visual.restype=C.c_int
native=[];recovered=[]
@Draw
def draw(_,resource,x,y,frame):recovered.append((resource,x,y,frame))
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def hook(uc,addr,size,data):
    if addr==0x413e10:
        uc.emu_stop();return
    if addr!=0x4105c0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    assert get(sp+16)==1 and get(sp+20)==0
    native.append((uc.reg_read(UC_X86_REG_ECX),*[C.c_int32(get(sp+i)).value for i in (4,8,12)]))
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
    state=(C.c_int32*len(values))(*values);cx=rng.randrange(-10000,10000);cy=rng.randrange(-10000,10000);field94=rng.randrange(2)
    u.mem_write(0x500000,bytes(state));put(0x530094,field94)
    put(0x510000,0x520000);put(0x510004,cx);put(0x510008,cy)
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x510000)
    native.clear();recovered.clear();u.emu_start(0x413cb0,0x520000,count=3000)
    result=lib.sniper_ship_visual(state,C.byref(blink),detail,7,field94,cx,cy,draw,draw,None)
    assert result==int(u.reg_read(UC_X86_REG_EIP)==0x413e10)
    assert blink.value==get(0x433850)
    assert native==recovered,(i,native,recovered)
    assert bytes(state)==bytes(u.mem_read(0x500000,0x218))
print('1000 integrated ship visual paths match original visibility, shared blink state, draw order and object mutations')
