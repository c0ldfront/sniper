"""412e50 call comparison, original draw4105c0 is captured."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int32,C.c_int32,C.c_int32,C.c_int,C.c_int)
ce=[];ne=[]
@Draw
def draw(_,asset,x,y,frame,center,opaque):ce.append((asset,x,y,frame,center,opaque))
lib=C.CDLL(str(ROOT/'code/build/libprojectile_draw.so'));lib.sniper_projectile_draw.argtypes=[C.POINTER(Projectile),C.c_int32,C.c_int32,C.c_int,Draw,C.c_void_p]
for i,a in enumerate([0x429310,0x428348,0x42927c,0x4289fc,0x428164]):put(a,0x510000+i*256)
def hook(uc,a,size,data):
    if a!=0x4105c0:return
    sp=uc.reg_read(UC_X86_REG_ESP);asset=(uc.reg_read(UC_X86_REG_ECX)-0x510000)//256
    ne.append((asset,*(get(sp+4+i*4) for i in range(5))))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+24)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x412e50);seen=set()
for case in range(1200):
    p=Projectile()
    for j in range(18):p.word[j]=rng.randint(-100,100)
    p.word[1]=rng.choice([-2147483648,-1,0,1,999,16000000,2147483647]);p.word[2]=rng.randint(-1000000,17000000)
    p.word[0x34//4]=case%6;p.word[0x38//4]=case%3;p.word[0x18//4]=rng.choice([-1,0,1,7,49,50,132,2147483647]);p.word[0x14//4]=rng.choice([-2147483648,-101,-1,0,6,10,23,99,2147483647])
    cx=rng.choice([-2147483648,0,2000,2147483647]);cy=rng.randint(-1000,16000);suppress=case%2
    u.mem_write(0x500000,bytes(p));put(0x422280,suppress);put(0x800000,0x810000);put(0x800004,cx);put(0x800008,cy)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000);ce.clear();ne.clear()
    u.emu_start(0x412e50,0x810000,count=10000);lib.sniper_projectile_draw(C.byref(p),cx,cy,suppress,draw,None)
    assert ce==ne,(case,ce,ne)
    seen.update(e[0] for e in ce)
assert seen==set(range(5)),seen
print('1200 original projectile draw comparisons pass across all5 assets,animation states,hidden/suppressed branches and signed coordinate/tick edges')
