"""4024f7..402598 draw layer dispatch,including callback mutations."""
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
class Pickup(C.Structure):_fields_=[('word',C.c_int32*5)]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
PickDraw=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Pickup),C.c_int32,C.c_int32)
ProjDraw=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Projectile),C.c_int32,C.c_int32)
Ships=C.CFUNCTYPE(None,C.c_void_p)
class World(C.Structure):_fields_=[('pickups',C.POINTER(C.POINTER(Pickup))),('pickup_count',C.POINTER(C.c_int32)),('projectiles',C.POINTER(C.POINTER(Projectile))),('projectile_count',C.POINTER(C.c_int32)),('x',C.POINTER(C.c_int32)),('y',C.POINTER(C.c_int32)),('pickup',PickDraw),('projectile',ProjDraw),('ships',Ships),('user',C.c_void_p)]
pickups=(Pickup*4)();projectiles=(Projectile*8)();pp=(C.POINTER(Pickup)*4)(*(C.pointer(p) for p in pickups));qp=(C.POINTER(Projectile)*8)(*(C.pointer(p) for p in projectiles))
pc=C.c_int32();qc=C.c_int32();x=C.c_int32();y=C.c_int32();ce=[];ne=[];mutation=False
@PickDraw
def pick(_,p,cx,cy):
    ce.append(('pickup',p.contents.word[0],cx,cy))
    if mutation:x.value+=1
@ProjDraw
def proj(_,p,cx,cy):
    ce.append(('projectile',p.contents.word[0],cx,cy))
    if mutation:p.contents.word[6]=1;y.value+=1
@Ships
def ships(_):
    ce.append(('ships',))
    if mutation:
        projectiles[0].word[6]=0
        if qc.value<8:qc.value+=1
w=World(pp,C.pointer(pc),qp,C.pointer(qc),C.pointer(x),C.pointer(y),pick,proj,ships,None)
lib=C.CDLL(str(ROOT/'code/build/libactor_layers.so'));lib.sniper_actor_layers.argtypes=[C.POINTER(World)]
def hook(uc,a,size,data):
    if a not in [0x412b20,0x412e50,0x406d00]:return
    sp=uc.reg_read(UC_X86_REG_ESP);obj=uc.reg_read(UC_X86_REG_ECX);clean=0
    if a==0x406d00:
        ne.append(('ships',))
        if mutation:
            put(0x510018,0)
            if get(0x4289f0)<8:put(0x4289f0,get(0x4289f0)+1)
    else:
        ne.append(('pickup' if a==0x412b20 else 'projectile',get(obj),get(sp+4),get(sp+8)));clean=8
        if mutation:
            if a==0x412b20:put(0x42de80,get(0x42de80)+1)
            else:put(obj+24,1);put(0x424324,get(0x424324)+1)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x4024f7)
for case in range(700):
    for i,p in enumerate(pickups):p.word[0]=i;u.mem_write(0x500000+i*20,bytes(p));put(0x429288+4*i,0x500000+i*20)
    for i,p in enumerate(projectiles):p.word[0]=i;p.word[6]=rng.choice([-1,0,1,49,132]);u.mem_write(0x510000+i*72,bytes(p));put(0x429748+4*i,0x510000+i*72)
    pc.value=case%5;qc.value=case%8;x.value=123;y.value=-44;mutation=case%3==0
    for a,v in [(0x428a08,pc.value),(0x4289f0,qc.value),(0x42de80,x.value),(0x424324,y.value)]:put(a,v)
    ce.clear();ne.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4024f7,0x402598,count=100000)
    lib.sniper_actor_layers(C.byref(w))
    assert ce==ne,(case,ce,ne)
    assert bytes(projectiles)==bytes(u.mem_read(0x510000,C.sizeof(projectiles))),case
    assert (pc.value,qc.value,x.value,y.value)==tuple(get(a) for a in [0x428a08,0x4289f0,0x42de80,0x424324]),case
print('700 native actor-layer blocks match pickup/projectile/ship ordering and live impact/count/camera mutations')
