"""Full412b60. Only positional sound is hooked; original x87 math executes."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX,UC_X86_REG_FPCW
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_int32)
class World(C.Structure):_fields_=[('local',C.POINTER(Ship)),('map',C.c_void_p),('projectiles',C.POINTER(C.POINTER(Projectile)))]+[(n,C.c_int32) for n in ['count','range','speed','duration','timer']]+[('bullets',C.c_int32*4),('bombs',C.c_int32*4),('repel_sound',C.c_int32),('sound',Sound),('user',C.c_void_p)]
ce=[];ne=[]
@Sound
def sound(_,handle,x,y):ce.append((handle,x,y))
s=Ship();p=Projectile();others=(Projectile*4)();ptrs=(C.POINTER(Projectile)*4)(*(C.pointer(q) for q in others));grid=(C.c_uint8*(1024*1024))()
w=World(C.pointer(s),C.addressof(grid),ptrs,4,100,2200,70,0,(C.c_int32*4)(11,12,13,14),(C.c_int32*4)(21,22,23,24),31,sound,None)
lib=C.CDLL(str(ROOT/'code/build/libprojectile_create.so'));lib.sniper_projectile_create.argtypes=[C.POINTER(Projectile),C.POINTER(World),C.POINTER(C.c_int32)]
for a,v in [(0x4218e0,0x510000),(0x429f48,0x600000),(0x4289f0,4),(0x4282d1,100),(0x4282c9,2200),(0x4282cd,70),(0x4218f8,31),(0x4230fc,0)]:put(a,v)
for i in range(4):put(0x428190+4*i,11+i);put(0x428168+4*i,21+i);put(0x429748+4*i,0x520000+72*i)
def hook(uc,a,size,data):
    if a!=0x407a80:return
    sp=uc.reg_read(UC_X86_REG_ESP);ne.append(tuple(get(sp+4+4*i) for i in range(3)))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x412b60);repelled=0
for case in range(1800):
    for j in range(18):p.word[j]=rng.randint(-100000,100000)
    C.memset(C.byref(s),0,536)
    s.word[0x48//4]=400000+rng.randint(-120000,120000);s.word[0x4c//4]=400000+rng.randint(-120000,120000);s.word[0x68//4]=case%2
    for q in others:
        for j in range(18):q.word[j]=rng.randint(-100,100)
        q.word[1]=400000+rng.randint(-120000,120000);q.word[2]=400000+rng.randint(-120000,120000);q.word[8]=rng.choice([0,1])
    C.memset(grid,0,1024*1024)
    if case%7==0:grid[(s.word[0x4c//4]//16000)*1024+s.word[0x48//4]//16000]=152
    w.timer=9;put(0x42930c,9)
    args=(C.c_int32*15)(7,400000,400000,17,-21,rng.randrange(40000),rng.randint(0,5000),case%6,case%4,2,1,case%10,123,0,25 if case%9==0 else 0)
    u.mem_write(0x500000,bytes(p));u.mem_write(0x510000,bytes(s));u.mem_write(0x520000,bytes(others));u.mem_write(0x600000,bytes(grid))
    put(0x800000,0x810000);u.mem_write(0x800004,bytes(args));u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_FPCW,0x37f)
    ce.clear();ne.clear();u.emu_start(0x412b60,0x810000,count=100000)
    lib.sniper_projectile_create(C.byref(p),C.byref(w),args)
    assert bytes(p)==bytes(u.mem_read(0x500000,72)),(case,'projectile',[(i,p.word[i],get(0x500000+4*i)) for i in range(18) if p.word[i]!=get(0x500000+4*i)])
    assert bytes(s)==bytes(u.mem_read(0x510000,536)),(case,'ship')
    assert bytes(others)==bytes(u.mem_read(0x520000,288)),(case,'other projectiles')
    assert ce==ne,(case,ce,ne)
    assert w.timer==get(0x42930c),(case,'timer')
    repelled+=w.timer==70
assert repelled>0
print(f'1800 full projectile constructors match all object/ship/projectile bytes,repel timer and sounds;{repelled} ship repels')
