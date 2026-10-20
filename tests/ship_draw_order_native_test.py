"""406d00 leader/visibility/order with live mutations during ship draw."""
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
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Draw=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,C.c_int32)
class World(C.Structure):_fields_=[('ships',C.POINTER(C.POINTER(Ship))),('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ['count','local_id','x','y']]+[('frame',C.c_uint32),('draw',Draw),('user',C.c_void_p)]
ships=(Ship*5)();ptrs=(C.POINTER(Ship)*5)();ce=[];ne=[];mutate=False
@Draw
def draw(_,s,x,y):
    ce.append((s.contents.word[17],x,y))
    if mutate:ships[0].word[0x94//4]=1;ships[0].word[0x6c//4]=1;w.x+=1
w=World(ptrs,C.pointer(ships[0]),5,0,0,0,0,draw,None)
lib=C.CDLL(str(ROOT/'code/build/libship_draw_order.so'));lib.sniper_ship_draw_order.argtypes=[C.POINTER(World)]
put(0x4218e0,0x500000);put(0x429318,0)
def hook(uc,a,size,data):
    if a!=0x413cb0:return
    sp=uc.reg_read(UC_X86_REG_ESP);obj=uc.reg_read(UC_X86_REG_ECX);ne.append((get(obj+0x44),get(sp+4),get(sp+8)))
    if mutate:put(0x500094,1);put(0x50006c,1);put(0x42de80,get(0x42de80)+1)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+12)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x406d00)
for case in range(1200):
    C.memset(ships,0,C.sizeof(ships));order=list(range(5));rng.shuffle(order)
    for i,s in enumerate(ships):
        for off,val in [(0x44,i),(0x6c,rng.choice([-1,1])),(0x68,rng.randrange(2)),(0x8c,rng.randrange(2)),(0x94,rng.randrange(2)),(0x80,rng.choice([-1,0,10,10,100])),(0x84,rng.randrange(2))]:s.word[off//4]=val
        u.mem_write(0x500000+536*i,bytes(s));ptrs[i]=C.pointer(ships[order[i]]);put(0x4283e8+4*i,0x500000+536*order[i])
    w.count=1 if case%9==0 else 5;w.frame=rng.choice([0,1,15,16,31,0xffffffff]);w.x=123;w.y=-99;mutate=case%7==0
    for a,v in [(0x429328,w.count),(0x4219a0,w.frame),(0x42de80,w.x),(0x424324,w.y),(0x800000,0x810000)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);ce.clear();ne.clear();u.emu_start(0x406d00,0x810000,count=100000)
    lib.sniper_ship_draw_order(C.byref(w))
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),case
    assert ce==ne,(case,ce,ne)
    assert w.frame==(get(0x4219a0)&0xffffffff) and w.x==get(0x42de80),case
print('1200 native ship draw passes match leader flags,cloak/radar visibility,attachment/local ordering and live callback changes')
