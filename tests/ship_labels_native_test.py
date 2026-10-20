"""Original label block including original CRT formatting; intercept final text draw."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(ROOT/'code/build/libship_labels.so'));Obj=C.c_int32*(0x218//4)
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32,C.c_int)
lib.sniper_ship_labels.argtypes=[C.POINTER(C.c_int32)]*3+[C.c_int,C.c_int32,C.c_int32,C.c_int32,C.c_int32,C.c_int32,Draw,C.c_void_p]
native=[];recovered=[]
@Draw
def draw(_,text,x,y,color):recovered.append((text,x,y,color))
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def hook(uc,a,size,data):
    if a!=0x415d60:return
    sp=uc.reg_read(UC_X86_REG_ESP);ptr=get(sp+4)
    text=bytes(uc.mem_read(ptr,128)).split(b'\0')[0]
    native.append((text,*[C.c_int32(get(sp+i)).value for i in (8,12,16)]))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
put(0x4218e0,0x501000);put(0x429318,7);put(0x429338+3*4,0x502000)
rng=random.Random(87)
for i in range(1000):
    o=Obj();local=Obj();parent=Obj();local[0x44//4]=7
    name=b'N'*rng.choice([1,30,31]);squad=b'S'*rng.choice([0,15,31])
    C.memmove(C.addressof(o),name,len(name));C.memmove(C.addressof(o)+32,squad,len(squad))
    for off,v in [(0x44,rng.choice([7,8])),(0x5c,rng.choice([-1,0,250000,250001,333332,333333])),(0xc8,1000000),(0x6c,rng.choice([-1,3,7])),(0x60,rng.choice([0,0,1])),(0x68,rng.choice([0,7])),(0xa0,rng.randrange(1000))]:o[off//4]=v
    local[0x68//4]=7;local[0xa0//4]=rng.randrange(1000);parent[0xc0//4]=rng.randrange(10)
    mode=rng.randrange(4);x=rng.randrange(-1000,1000);y=rng.randrange(-1000,1000)
    for a,state in [(0x500000,o),(0x501000,local),(0x502000,parent)]:u.mem_write(a,bytes(state))
    for a,v in [(0x428344,mode),(0x4282bd,50),(0x4282c1,35),(0x4282c5,10)]:put(a,v)
    u.reg_write(UC_X86_REG_ESI,0x500000);u.reg_write(UC_X86_REG_EDI,x&0xffffffff);u.reg_write(UC_X86_REG_EBP,y&0xffffffff);u.reg_write(UC_X86_REG_ESP,0x510000)
    native.clear();recovered.clear();u.emu_start(0x413e10,0x413f8e,count=100000)
    lib.sniper_ship_labels(o,local,parent,mode,50,35,10,x,y,draw,None)
    assert native==recovered,(i,native,recovered)
    assert bytes(parent)==bytes(u.mem_read(0x502000,0x218))
print('1000 label blocks match original text, placement, color and attachment row mutation')
