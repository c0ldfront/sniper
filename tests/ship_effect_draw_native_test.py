"""Compare ordered draw arguments from original full particle renderer."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(ROOT/'code/build/libship_effect_draw.so'))
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int32,C.c_int32,C.c_int32)
lib.sniper_ship_effect_draw.argtypes=[C.POINTER(C.c_int32),C.c_int32,C.c_int32,C.c_int32,Draw,C.c_void_p]
lib.sniper_ship_effect_draw.restype=None
native=[];recovered=[]
@Draw
def draw(_,resource,x,y,frame):recovered.append((resource,x,y,frame))
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def hook(uc,addr,size,data):
    if addr!=0x4105c0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    assert get(sp+16)==1 and get(sp+20)==0
    native.append((uc.reg_read(UC_X86_REG_ECX),*[C.c_int32(get(sp+i)).value for i in (4,8,12)]))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+24)
u.hook_add(UC_HOOK_CODE,hook)
put(0x42817c,173);put(0x428180,147);put(0x429318,7)
rng=random.Random(87)
for i in range(500):
    values=[rng.randrange(-100000,100001) for _ in range(0x218//4)];values[0x44//4]=rng.choice([7,8])
    for j in range(12):
        values[0x15c//4+j*4]=rng.choice([-1,0,1,2,3,51,52,53,56,57,58])
        values[0x160//4+j*4]=rng.choice([0,1,2,-1])
    state=(C.c_int32*len(values))(*values);cx=rng.randrange(-10000,10000);cy=rng.randrange(-10000,10000)
    u.mem_write(0x500000,bytes(state))
    for j,v in enumerate([0x520000,cx,cy]):put(0x510000+j*4,v)
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x510000)
    native.clear();recovered.clear();u.emu_start(0x413c20,0x520000,count=2000)
    lib.sniper_ship_effect_draw(state,7,cx,cy,draw,None)
    assert native==recovered,(i,native,recovered)
    assert bytes(state)==bytes(u.mem_read(0x500000,0x218))
print('500 full particle-render calls match original ordered sprite arguments and preserve state')
