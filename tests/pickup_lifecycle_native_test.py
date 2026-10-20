"""Original404f10 creation and4021d5 frame expiry loop with real object methods."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,4096);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
Release=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Pickup));Allocate=C.CFUNCTYPE(C.c_void_p,C.c_void_p)
class World(C.Structure):
    _fields_=[('items',C.POINTER(C.POINTER(Pickup))),('count',C.c_int32),('map',C.POINTER(C.c_uint8)),('local',C.c_void_p),
        ('sharing_enabled',C.c_int32),('sharing_limit',C.c_int32),('lookup',C.c_void_p),('release',Release),('award',C.c_void_p),('user',C.c_void_p)]
objects=(Pickup*17)();pointers=(C.POINTER(Pickup)*17)();grid=(C.c_uint8*(1024*1024))();cevents=[];nevents=[];fail=False
@Allocate
def allocate(_):cevents.append(('allocate',20));return None if fail else C.addressof(objects[16])
@Release
def release(_,p):
    o=p.contents;cevents.append(('release',o.x,w.count,grid[o.y*1024+o.x]))
w=World(pointers,0,grid,None,0,0,None,release,None,None)
lib=C.CDLL(str(ROOT/'code/build/libpickup_lifecycle.so'))
lib.sniper_pickup_create.argtypes=[C.POINTER(World)]+[C.c_int32]*4+[Allocate,C.c_void_p]
lib.sniper_pickup_expire.argtypes=[C.POINTER(World),C.c_int32]
def hook(uc,a,size,data):
    if a not in [0x416b20,0x416620]:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x416b20:
        nevents.append(('allocate',get(sp+4)));uc.reg_write(UC_X86_REG_EAX,0x530200)
    else:
        p=get(sp+4);x,y=get(p+4),get(p+8);nevents.append(('release',x,get(0x428a08),u.mem_read(0x600000+y*1024+x,1)[0]))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook);put(0x429f48,0x600000)
rng=random.Random(412);created=expired=0
for case in range(350):
    C.memset(grid,0,len(grid));w.count=case%17
    for i in range(17):
        objects[i]=Pickup(rng.choice([0,0x7fffffff,-0x80000000]),20+i,30+i,rng.choice([-1,0,1,10,50,51,0x7fffffff,-0x80000000]),i+1)
        pointers[i]=C.pointer(objects[i]);u.mem_write(0x530000+i*32,bytes(objects[i]));put(0x429288+i*4,0x530000+i*32)
        if i<w.count:grid[(30+i)*1024+20+i]=255
    x,y=[(500,500),(-1,0),(1024,0),(0,-1),(0,1024),(0,0),(1023,1023),(20,30)][case%8]
    if case%9==0 and 0<=x<1024 and 0<=y<1024:grid[y*1024+x]=152
    code=rng.randint(-21,21);life=rng.randint(-1,1000)
    put(0x428a08,w.count);u.mem_write(0x600000,bytes(grid))
    for i,v in enumerate([0x810000,x,y,code,life]):put(0x800000+i*4,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);cevents.clear();nevents.clear()
    u.emu_start(0x404f10,0x810000,count=100000)
    result=lib.sniper_pickup_create(C.byref(w),x,y,code,life,allocate,None)
    assert result in [0,1];created+=result
    assert w.count==get(0x428a08);assert cevents==nevents
    assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid)))
    for i in range(w.count):assert bytes(pointers[i].contents)==bytes(u.mem_read(get(0x429288+4*i),20))
    # Same objects/list/map continue directly into expiry.
    dt=[0,1,10,50,0x7fffffff][case%5];put(0x4218c8,dt)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EAX,0)
    cevents.clear();nevents.clear();u.emu_start(0x4021d5,0x402237,count=100000)
    lib.sniper_pickup_expire(C.byref(w),dt)
    assert w.count==get(0x428a08);assert cevents==nevents,(case,cevents,nevents)
    assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid)))
    for i in range(w.count):assert bytes(pointers[i].contents)==bytes(u.mem_read(get(0x429288+4*i),20))
    expired+=len(cevents)
# Documented allocation-failure deviation: leave count/map unchanged.
fail=True;w.count=0;grid[500*1024+500]=0
assert lib.sniper_pickup_create(C.byref(w),500,500,1,100,allocate,None)==-1
assert w.count==0 and grid[500*1024+500]==0
print(f'350 creation/expiry sequences match original objects,list,map and callbacks; {created} creations,{expired} expirations; allocation failure separately checked')
