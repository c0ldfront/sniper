"""Original two-pass loop,including live attachment/list/count/tick mutations."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Update=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32)
ships=(Ship*8)();ordered=(C.POINTER(Ship)*8)();count=C.c_int32();ticks=C.c_int32();ce=[];ne=[];mode=0
# Mutate only on first call, preserving original synchronous semantics.
def mutate(native):
    if mode==1:
        if native:put(0x50006c,2)
        else:ships[0].word[0x6c//4]=2
    elif mode==2:
        if native:put(0x500000+536+0x6c,-1)
        else:ships[1].word[0x6c//4]=-1
    elif mode==3:
        if native:put(0x429328,max(0,get(0x429328)-1))
        else:count.value=max(0,count.value-1)
    elif mode==4:
        if native:put(0x429328,8)
        else:count.value=8
    elif mode==5:
        if native:put(0x4218c8,49)
        else:ticks.value=49
    elif mode==6:
        if native:
            a,b=get(0x4283e8+4),get(0x4283e8+8);put(0x4283e8+4,b);put(0x4283e8+8,a)
        else:
            a=C.addressof(ordered[1].contents);b=C.addressof(ordered[2].contents)
            ordered[1]=C.cast(b,C.POINTER(Ship));ordered[2]=C.cast(a,C.POINTER(Ship))
@Update
def update(_,ship,dt):
    ce.append((ship.contents.word[0x44//4],dt))
    if len(ce)==1:mutate(False)
def hook(uc,a,size,data):
    if a!=0x413fa0:return
    sp=uc.reg_read(UC_X86_REG_ESP);p=uc.reg_read(UC_X86_REG_ECX)
    ne.append((get(p+0x44),get(sp+4)))
    if len(ne)==1:mutate(True)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
u.hook_add(UC_HOOK_CODE,hook)
lib=C.CDLL(str(ROOT/'code/build/libship_update_order.so'))
lib.sniper_ship_update_order.argtypes=[C.POINTER(C.POINTER(Ship)),C.POINTER(C.c_int32),C.POINTER(C.c_int32),Update,C.c_void_p]
rng=random.Random(402237);cases=0
for mode in range(7):
    for trial in range(100):
        for i in range(8):
            ships[i].word[0x44//4]=100+i;ships[i].word[0x6c//4]=rng.choice([-1,2])
            u.mem_write(0x500000+536*i,bytes(ships[i]))
        order=list(range(8));rng.shuffle(order)
        for i,j in enumerate(order):ordered[i]=C.pointer(ships[j]);put(0x4283e8+4*i,0x500000+536*j)
        count.value=rng.randrange(9);ticks.value=rng.randrange(51)
        put(0x429328,count.value);put(0x4218c8,ticks.value);ce.clear();ne.clear()
        u.reg_write(UC_X86_REG_ESP,0x510000);u.emu_start(0x402237,0x40229b,count=10000)
        lib.sniper_ship_update_order(ordered,C.byref(count),C.byref(ticks),update,None)
        assert ce==ne,(mode,trial,ce,ne)
        assert count.value==get(0x429328) and ticks.value==get(0x4218c8)
        for i in range(8):assert bytes(ships[i])==bytes(u.mem_read(0x500000+536*i,536))
        cases+=1
print(f'{cases} two-pass ship-update sequences match original order and live mutations')
