import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX
ROOT=pathlib.Path(__file__).resolve().parents[2];u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_int32,C.c_int,C.c_int)
ce=[];ne=[]
@Draw
def draw(_,x,y,frame,centered,opaque):ce.append((x,y,frame,centered,opaque))
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def hook(uc,a,size,data):
    if a!=0x4105c0:return
    assert uc.reg_read(UC_X86_REG_ECX)==0x550000
    sp=uc.reg_read(UC_X86_REG_ESP);ne.append(tuple(get(sp+4*i) for i in range(1,6)))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+24)
u.hook_add(UC_HOOK_CODE,hook);put(0x428a64,0x550000)
lib=C.CDLL(str(ROOT/'code/build/libpickup_draw.so'));lib.sniper_pickup_draw.argtypes=[C.POINTER(Pickup),C.c_int32,C.c_int32,Draw,C.c_void_p]
rng=random.Random(412);ages=list(range(241))+[-1,-12,-120,-0x80000000,0x7fffffff]
for age in ages:
    p=Pickup(age,rng.randrange(1024),rng.randrange(1024),0,0);x=C.c_int32(rng.getrandbits(32)).value;y=C.c_int32(rng.getrandbits(32)).value
    u.mem_write(0x500000,bytes(p));put(0x510000,0x520000);put(0x510004,x);put(0x510008,y)
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x510000);ce.clear();ne.clear()
    u.emu_start(0x412b20,0x520000,count=1000);lib.sniper_pickup_draw(C.byref(p),x,y,draw,None);assert ce==ne
print(f'{len(ages)} pickup draw calls match original coordinates,frame and blit modes')
