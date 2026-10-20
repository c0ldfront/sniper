"""Main frame clock/order/gates and actual camera; stage bodies controlled."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Clock(C.Structure):_fields_=[(n,C.c_uint32) for n in ('previous','elapsed','tick')]
class Camera(C.Structure):_fields_=[('x',C.c_int32),('y',C.c_int32)]
Call=C.CFUNCTYPE(None,C.c_void_p)
Value=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
class Frame(C.Structure):
    _fields_=[('clock',Clock),('local',C.POINTER(Ship)),('watched',C.POINTER(Ship))]+[(n,C.c_int32) for n in ('auto','special','safe','frequency','frequency_count','width','height','tile_x','tile_y','shake')]+[('rng',C.c_uint32),('camera',Camera)]+[(n,Call) for n in ('post','backend','input')]+[('team',Value),('detach',Value),('simulate',Call),('user',C.c_void_p)]
ce=[];ne=[];mutate=False;special=0;safe=0;frequency=0;attached=-1;shake=0
@Call
def post(_):ce.append(('post',))
@Call
def backend(_):ce.append(('backend',frame.clock.elapsed,frame.clock.tick))
@Call
def input_(_):
    ce.append(('input',))
    if mutate:frame.special=special;frame.safe=safe;frame.frequency=frequency
@Value
def team(_,v):ce.append(('team',v));ship.word[0x6c//4]=attached
@Value
def detach(_,v):ce.append(('detach',v));ship.word[0x6c//4]=-1
@Call
def simulate(_):ce.append(('simulate',frame.safe,frame.frequency));frame.shake=shake
lib=C.CDLL(str(ROOT/'code/build/libframe_update.so'));lib.sniper_frame_update.argtypes=[C.POINTER(Frame),C.c_uint32]
for a,v in [(0x4218e0,0x500000),(0x424340,1),(0x424348,0x520000),(0x520000,0x520100),(0x520108,0x530000),(0x520128,0x530100),(0x52012c,0x530200),(0x4354d0,0x530300),(0x422240,0x1234)]:put(a,v)
def hook(uc,a,size,user):
    if a==0x4021bf:
        ne.append(('simulate',get(0x4283d8),get(0x42def0)));put(0x429738,shake)
        uc.reg_write(UC_X86_REG_EIP,0x402338);return
    if a not in (0x530000,0x530100,0x530200,0x530300,0x4050c0):return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x530000:ne.append(('backend',get(0x4218c8)&0xffffffff,get(0x4218cc)&0xffffffff))
    elif a==0x4050c0:
        ne.append(('input',))
        if mutate:
            for addr,v in [(0x4218d0,special),(0x4283d8,safe),(0x42def0,frequency)]:put(addr,v)
    elif a==0x530100:ne.append(('team',get(sp+4)));put(0x50006c,attached);clean=4
    elif a==0x530200:ne.append(('detach',get(sp+4)));put(0x50006c,-1);clean=4
    else:
        assert [get(sp+i) for i in (4,8,12,16)]==[0x1234,0x401,0,0]
        ne.append(('post',));clean=16
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x402130);posts=teams=skips=0
for case in range(1400):
    ship=Ship();watched=Ship()
    for s in (ship,watched):
        s.word[0x48//4]=rng.randrange(-2000000,16000000);s.word[0x4c//4]=rng.randrange(-2000000,16000000)
    ship.word[0x40//4]=case%5;ship.word[0x5c//4]=rng.choice([-1,500000,500001]);ship.word[0x6c//4]=rng.choice([-1,0,1])
    ms=rng.getrandbits(32);previous=(ms//10-rng.choice([-1,0,1,49,50,51,1000]))&0xffffffff
    frame=Frame(Clock(previous,0,rng.getrandbits(32)),C.pointer(ship),C.pointer(watched) if case%2 else None,
        case%2,case%3==0,rng.choice([-1,0,1,40,100]),rng.randrange(-2000,2000),rng.choice([1,2,16,1024]),
        rng.choice([320,641,1024]),rng.choice([240,481,768]),rng.randrange(1024),rng.randrange(1024),rng.randrange(100),rng.getrandbits(32),Camera(),post,backend,input_,team,detach,simulate,None)
    mutate=bool(case%2);special=case%4==0;safe=rng.choice([-1,0,1,40,100]);frequency=rng.randrange(-2000,2000);attached=rng.choice([-1,0,1]);shake=rng.choice([-1,0,1,40])
    u.mem_write(0x500000,bytes(ship));u.mem_write(0x510000,bytes(watched));put(0x42933c,0x510000 if frame.watched else 0)
    for a,v in [(0x42e198,previous),(0x4218cc,frame.clock.tick),(0x428160,frame.auto),(0x4218d0,frame.special),(0x4283d8,frame.safe),(0x42def0,frame.frequency),(0x4282a9,frame.frequency_count),(0x42818c,frame.width),(0x42434c,frame.height),(0x4218d4,frame.tile_x),(0x4218d8,frame.tile_y),(0x429738,frame.shake),(0x422f78,frame.rng)]:put(a,v)
    ce.clear();ne.clear();u.reg_write(UC_X86_REG_EAX,ms);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x4020b5,0x402468,count=10000)
    lib.sniper_frame_update(C.byref(frame),ms)
    assert ce==ne,(case,ce,ne)
    assert [frame.clock.previous,frame.clock.elapsed,frame.clock.tick]==[get(a)&0xffffffff for a in (0x42e198,0x4218c8,0x4218cc)]
    assert [frame.safe,frame.frequency,frame.shake,frame.camera.x,frame.camera.y]==[get(a) for a in (0x4283d8,0x42def0,0x429738,0x42de80,0x424324)]
    assert frame.rng==get(0x422f78)&0xffffffff and bytes(ship)==bytes(u.mem_read(0x500000,536))
    posts+=sum(e[0]=='post' for e in ce);teams+=sum(e[0]=='team' for e in ce);skips+=not any(e[0]=='simulate' for e in ce)
print(f'1400 main frame traces/state/camera comparisons pass;{posts} posted commands,{teams} team changes,{skips} skipped simulations; stage bodies controlled')
