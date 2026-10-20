"""406922..406a84, backend+0c captured with post-callback mutation."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ESI
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Send=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int32))
class World(C.Structure):_fields_=[('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ['interval','ticks','last_ticks','last_vx','last_vy','last_heading','dirty']]+[('send',Send),('user',C.c_void_p)]
ce=[];ne=[];mutate=False
s=Ship()
@Send
def send(_,args):
    ce.append(tuple(args[i] for i in range(13)))
    if mutate:w.ticks+=3;s.word[0x50//4]+=1;s.word[0x58//4]+=1000;w.dirty=9
w=World(C.pointer(s),10,0,0,0,0,0,0,send,None)
lib=C.CDLL(str(ROOT/'code/build/libposition_dispatch.so'));lib.sniper_position_dispatch.argtypes=[C.POINTER(World),C.POINTER(C.c_int32)]
for a,v in [(0x4218e0,0x500000),(0x424348,0x520000),(0x520000,0x520100),(0x52010c,0x530000)]:put(a,v)
def hook(uc,a,size,data):
    if a!=0x530000:return
    sp=uc.reg_read(UC_X86_REG_ESP);ne.append(tuple(get(sp+4+i*4) for i in range(13)))
    if mutate:put(0x4218cc,get(0x4218cc)+3);put(0x500050,get(0x500050)+1);put(0x500058,get(0x500058)+1000);put(0x4292fc,9)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+56)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x406922);sent=0
for case in range(1800):
    C.memset(C.byref(s),0,536)
    for off,val in [(0x48,rng.choice([-1,0,123456])),(0x4c,-123456),(0x50,rng.choice([-10000,0,499,500,10000])),(0x54,rng.choice([-10000,0,499,500])),(0x58,rng.choice([-1999,12345,39999])),(0x6c,rng.choice([-1,-1,1])),(0xa0,123)]:s.word[off//4]=val
    for off in [0x88,0x8c,0x90,0x94]:s.word[off//4]=rng.choice([0,1,-1])
    w.interval=rng.choice([0,10,2147483647]);w.ticks=rng.choice([-2147483648,0,9,10,39,40,100,2147483647]);w.last_ticks=rng.choice([-1,0,10,2147483647])
    w.last_vx=s.word[0x50//4]+(case%3==0);w.last_vy=s.word[0x54//4]+(case%7==0);w.last_heading=s.word[0x58//4]+(case%11==0);w.dirty=case%2
    weapon=(C.c_int32*6)(rng.choice([0,0,1,3,5]),2,1,2,7,1)
    u.mem_write(0x500000,bytes(s))
    addresses=[0x428205,0x4218cc,0x4281c0,0x428a68,0x428a00,0x428a70,0x4292fc]
    for a,name in zip(addresses,['interval','ticks','last_ticks','last_vx','last_vy','last_heading','dirty']):put(a,getattr(w,name))
    for off,v in zip([0x20,0x1c,0x14,0x10,0x18],list(weapon)[1:]):put(0x800000+off,v)
    mutate=case%4==0;u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ESI,weapon[0]);ce.clear();ne.clear()
    u.emu_start(0x406922,0x406a84,count=100000)
    result=lib.sniper_position_dispatch(C.byref(w),weapon)
    assert result==bool(ne),(case,'result')
    assert ce==ne,(case,ce,ne)
    assert [getattr(w,n) for n in ['interval','ticks','last_ticks','last_vx','last_vy','last_heading','dirty']]==[get(a) for a in addresses],(case,'globals')
    assert bytes(s)==bytes(u.mem_read(0x500000,536)),(case,'ship')
    sent+=result
assert 0<sent<1800
print(f'1800 original position-dispatch blocks match timing,13 backend arguments,dirty flag and post-callback snapshots;{sent} sends')
