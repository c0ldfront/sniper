"""Compare whole-object changes from original reset block, including untouched fields."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Defaults(C.Structure):_fields_=[(n,C.c_int32) for n in ['recharge','energy_maximum','rotation','acceleration','speed','rotation_bonus','acceleration_bonus','speed_bonus','energy_bonus','recharge_bonus']]
lib=C.CDLL(str(ROOT/'code/build/libship_create.so'));lib.sniper_ship_reset.argtypes=[C.POINTER(C.c_int32),C.POINTER(Defaults)];lib.sniper_ship_reset.restype=None
addresses=[0x428251,0x42824d,0x428241,0x428245,0x428249,0x428255,0x428259,0x42825d,0x428261,0x428265]

def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
clock=0
def hook(uc,addr,size,data):
    if addr!=0x820000:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    uc.reg_write(UC_X86_REG_EAX,clock)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
put(0x4353f4,0x820000);put(0x429f48,0x600000)

class Identity(C.Structure):
    _fields_=[('player_id',C.c_int32),('name',C.c_char_p),('squad',C.c_char_p)]+[(n,C.c_int32) for n in ['type','field78','field7c','field80','team','attached_id','field70']]
lib.sniper_ship_create.argtypes=[C.POINTER(C.c_int32),C.POINTER(Identity),C.POINTER(Defaults),C.c_uint32]
lib.sniper_ship_create.restype=None
rng=random.Random(87)
for i in range(400):
    values=[rng.randrange(-2**31,2**31) for _ in range(0x218//4)]
    state=(C.c_int32*len(values))(*values)
    params=[rng.randrange(1,200000) for _ in addresses];d=Defaults(*params)
    name=b'N'*rng.choice([0,1,30,31,32,60]);squad=b'S'*rng.choice([0,1,30,31])
    id=Identity(i,name,squad,rng.randrange(5),11,22,33,7,-1,99)
    u.mem_write(0x500000,bytes(state));u.mem_write(0x700000,name+b'\0');u.mem_write(0x701000,squad+b'\0')
    for a,v in zip(addresses,params):put(a,v)
    clock=rng.getrandbits(32)
    for j,v in enumerate([0x810000,i,0x700000,0x701000,id.type,11,22,33,7,-1,99]):put(0x800000+4*j,v)
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x4136d0,0x810000,count=10000)
    assert u.reg_read(UC_X86_REG_EIP)==0x810000
    lib.sniper_ship_create(state,C.byref(id),C.byref(d),clock)
    assert bytes(state)==bytes(u.mem_read(0x500000,len(bytes(state)))),i
print('400 full ship constructors match all536 bytes, including string padding and untouched fields')
