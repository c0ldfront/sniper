"""Frame stage order and presentation retries from original4024c3..40269b."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
STACK,SHIP,PRIMARY,VT,FLIP=[0x1000000+i*65536 for i in range(5)]
for a in (STACK,SHIP,PRIMARY,VT,FLIP):u.mem_map(a,65536)
def word(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def words(a,n):return struct.unpack('<'+'I'*n,u.mem_read(a,n*4))
word(0x4218e0,SHIP);word(0x42dd6c,PRIMARY);word(PRIMARY,VT);word(VT+0x2c,FLIP);word(0x42931c,1234)
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Stage=C.CFUNCTYPE(None,C.c_void_p,C.c_int)
Flip=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
Restore=C.CFUNCTYPE(C.c_int,C.c_void_p)
class Frame(C.Structure):_fields_=[(n,C.c_int32) for n in ('stars','special','radar','chat','mode')]+[('local',C.POINTER(Ship)),('stage',Stage),('flip',Flip),('restore',Restore),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libscene_frame.so'));lib.sniper_scene_frame.argtypes=[C.POINTER(Frame)]
actual=[];native=[];mutations={};sequence=[];restored=1;ni=ci=0;native_result=None
field_addrs={'stars':0x42e018,'special':0x4218d0,'radar':0x429308,'chat':0x42819c,'mode':0x4282ec,'ship_type':SHIP+0x40}
def mutate(stage,is_native):
    for name,value in mutations.get(stage,[]):
        if is_native:word(field_addrs[name],value)
        elif name=='ship_type':ship.word[0x40//4]=value
        else:setattr(frame,name,value)
@Stage
def stage(_,value):actual.append(value);mutate(value,False)
@Flip
def flip(_):
    global ci
    actual.append('flip');result=sequence[ci];ci+=1;return result
@Restore
def restore(_):actual.append('restore');return restored
calls={0x409840:0,0x4107b0:1,0x406b70:2,0x406ea0:3,0x407c00:5,0x410840:6,0x4071b0:7,0x408e70:8,0x408700:10,0x408970:12}
def intercept(uc,a,size,user):
    global ni,native_result
    if a in (0x402677,0x402696):native_result=int(a==0x402696);uc.emu_stop();return
    if a in (0x4024f7,0x4025c9):
        event=4 if a==0x4024f7 else 9;native.append(event);mutate(event,True)
        uc.reg_write(UC_X86_REG_EIP,0x402598 if event==4 else 0x402626);return
    if a not in calls and a not in (FLIP,0x403e40,0x408ac0):return
    sp=uc.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0];pop=0;result=0
    if a==FLIP:
        assert words(sp+4,3)==(PRIMARY,0,1);native.append('flip');result=sequence[ni];ni+=1;pop=3
    elif a==0x403e40:native.append('restore');result=restored
    else:
        event=11 if a==0x408ac0 and words(sp+4,1)[0]==0 else 13 if a==0x408ac0 else calls[a]
        if a==0x4107b0:assert words(sp+4,1)[0]==1234
        native.append(event);mutate(event,True)
    uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4+4*pop);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,intercept)
rng=random.Random(87);failed=0
for case in range(1000):
    ship=Ship();ship.word[0x40//4]=rng.choice([0,1,4])
    frame=Frame(rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(-1,7),C.pointer(ship),stage,flip,restore,None)
    mutations={}
    if case%2:
        for event in [0,1,2,4,5,6,7,8,9]:
            name=rng.choice(list(field_addrs));value=rng.randrange(-1,6) if name in ('mode','ship_type') else rng.randrange(2)
            mutations[event]=[(name,value)]
    sequence=[rng.choice([0x8876021c,0x80004005,0x887601c2]) for _ in range(rng.randrange(5))]+[0]
    restored=rng.randrange(2);ni=ci=0;native_result=None;actual.clear();native.clear()
    for name,a in field_addrs.items():word(a,ship.word[0x40//4] if name=='ship_type' else getattr(frame,name))
    u.reg_write(UC_X86_REG_ESP,STACK+32768)
    u.emu_start(0x4024c3,0x40269b,count=100000)
    assert native_result is not None
    result=lib.sniper_scene_frame(C.byref(frame))
    assert result==native_result and actual==native,(case,result,native_result,actual,native)
    failed+=not result
print(f'1000 scene stage/presentation traces match original; live flag mutations and {failed} failed recoveries included')
