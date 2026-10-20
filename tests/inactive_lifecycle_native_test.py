"""Native inactive dispatch and activation/erase gates; OS/backend calls observed."""
import ctypes as C
import pathlib, random, struct, subprocess, tempfile
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Send=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int32))
class Position(C.Structure):
    _fields_=[('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ['interval','ticks','last_ticks','last_vx','last_vy','last_heading','position_dirty']]+[('send',Send),('user',C.c_void_p)]
ce=[];ne=[];events=[]
@Send
def send(_,args):ce.append(tuple(args[i] for i in range(13)))
def hook(uc,a,size,_):
    if a not in [0x530000,0x530100,0x530200,0x530300,0x530400]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x530000:
        ne.append(tuple(get(sp+4+4*i) for i in range(13)));clean=52
    elif a==0x530100:events.append('clock');uc.reg_write(UC_X86_REG_EAX,1234567)
    elif a==0x530200:events.append(('capture',get(sp+4)));clean=4
    elif a==0x530300:events.append('release')
    else:events.append(('stop',get(sp+4)));clean=4
    uc.reg_write(UC_X86_REG_EIP,get(sp)&0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
for a,v in [(0x424348,0x520000),(0x520000,0x520100),(0x52010c,0x530000),(0x4218e0,0x500000),
            (0x4353f4,0x530100),(0x435570,0x530200),(0x43556c,0x530300),(0x422240,123)]:put(a,v)
with tempfile.TemporaryDirectory() as temp:
    so=pathlib.Path(temp)/'inactive.so'
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror','-o',str(so),str(ROOT/'code/src/inactive_position.c')],check=True)
    lib=C.CDLL(str(so));lib.sniper_inactive_position.argtypes=[C.POINTER(Position)]
    rng=random.Random(0x4050d4)
    for case in range(1800):
        ship=Ship()
        for i in range(134):ship.word[i]=C.c_int32(rng.getrandbits(32)).value
        for offset in [0x88,0x8c,0x90,0x94]:ship.word[offset//4]=rng.choice([0,1,-1])
        p=Position(C.pointer(ship),*[C.c_int32(rng.getrandbits(32)).value for _ in range(7)],send,None)
        if case%2:p.interval=rng.randrange(100);p.last_ticks=p.ticks-rng.randrange(500)
        fields=[(0x428205,p.interval),(0x4218cc,p.ticks),(0x4281c0,p.last_ticks),(0x428a68,p.last_vx),
                (0x428a00,p.last_vy),(0x428a70,p.last_heading),(0x4292fc,p.position_dirty)]
        for a,v in fields:put(a,v)
        put(0x422234,0);put(0x800000,0x540000);u.mem_write(0x500000,bytes(ship))
        u.reg_write(UC_X86_REG_ESP,0x800000);ce.clear();ne.clear()
        u.emu_start(0x4050c0,0x540000,count=10000)
        result=lib.sniper_inactive_position(C.byref(p))
        assert ce==ne and result==bool(ne),case
        assert [p.last_ticks,p.last_vx,p.last_vy,p.last_heading,p.position_dirty]==[get(a) for a in [0x4281c0,0x428a68,0x428a00,0x428a70,0x4292fc]],case
        assert bytes(ship)==bytes(u.mem_read(0x500000,536))

# Actual WM_ACTIVATEAPP body including actual40a840; only external APIs intercepted.
for present in [0,1]:
    for running in [0,1]:
        for active in [0,1,2]:
            for a,v in [(0x42debc,present),(0x42deb0,77),(0x42dec4,2),(0x42dec8,0x550000),
                (0x550000,0x551000),(0x550004,0x552000),(0x551044,2),(0x552044,1),
                (0x551000,0x553000),(0x551004,0x554000),(0x552000,0x555000),
                (0x553000,0x556000),(0x554000,0x556000),(0x555000,0x556000),(0x556048,0x530400),
                (0x42223c,running),(0x42e198,999),(0x810010,active)]:put(a,v)
            events.clear();u.reg_write(UC_X86_REG_EBP,0x810000);u.reg_write(UC_X86_REG_ESP,0x800000)
            u.emu_start(0x40b086,0x40cddb,count=10000)
            expected=[]
            if present and not active:expected=[('stop',x) for x in [0x553000,0x554000,0x555000]]
            if active:
                if running:expected.append(('capture',123))
                expected.append('clock')
            elif running:expected.append('release')
            assert events==expected,(present,running,active,events)
            assert get(0x422234)==active and get(0x42deb0)==(active if present else 77)
            assert get(0x42e198)==(123456 if active else 999)
for running in [0,1]:
    put(0x42223c,running)
    u.emu_start(0x40b06f,0x40b07c if running else 0x40cddb,count=100)
    if running:
        u.emu_start(0x40b07c,0x40cdf1,count=100)
        assert u.reg_read(UC_X86_REG_EAX)==1
print('PASS1800 inactive dispatch comparisons;12 actual activation/audio/capture/rebase traces;2 erase-background gates')
