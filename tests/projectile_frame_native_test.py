"""40229b frame-loop control: callback mutation, append, remove, tick rereads."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
Step=C.CFUNCTYPE(C.c_int,C.c_void_p,C.POINTER(Projectile),C.c_int32)
Release=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Projectile))
objects=(Projectile*32)();items=(C.POINTER(Projectile)*32)();count=C.c_int32();ticks=C.c_int32();ce=[];ne=[];actions={};release_append=False
@Step
def step(_,p,dt):
    i=p.contents.word[0];ce.append(('step',i,dt));remove,append,change,replace=actions[i]
    if append>=0:items[count.value]=C.pointer(objects[append]);count.value+=1
    if change:ticks.value+=3
    if replace>=0:items[0]=C.pointer(objects[replace])
    return remove
@Release
def release(_,p):
    ce.append(('release',p.contents.word[0],count.value))
    if release_append:items[count.value]=C.pointer(objects[31]);count.value+=1
lib=C.CDLL(str(ROOT/'code/build/libprojectile_frame.so'));lib.sniper_projectile_frame.argtypes=[C.POINTER(C.POINTER(Projectile)),C.POINTER(C.c_int32),C.POINTER(C.c_int32),Step,Release,C.c_void_p]
def hook(uc,a,size,data):
    if a not in [0x413010,0x416620,0x416280]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;result=0
    if a==0x413010:
        i=get(uc.reg_read(UC_X86_REG_ECX));dt=get(sp+4);ne.append(('step',i,dt));result,append,change,replace=actions[i];clean=4
        if append>=0:put(0x429748+get(0x4289f0)*4,0x500000+append*72);put(0x4289f0,get(0x4289f0)+1)
        if change:put(0x4218c8,get(0x4218c8)+3)
        if replace>=0:put(0x429748,0x500000+replace*72)
    elif a==0x416620:
        ne.append(('release',get(get(sp+4)),get(0x4289f0)))
        if release_append:put(0x429748+get(0x4289f0)*4,0x500000+31*72);put(0x4289f0,get(0x4289f0)+1)
    else:
        dst,src,size=get(sp+4),get(sp+8),get(sp+12)
        if size:u.mem_write(dst,bytes(u.mem_read(src,size)))
    uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x40229b);appended_visits=0
for case in range(700):
    for i,p in enumerate(objects):p.word[0]=i;u.mem_write(0x500000+i*72,bytes(p))
    C.memset(items,0,C.sizeof(items));u.mem_write(0x429748,bytes(128));count.value=case%9;ticks.value=case%51
    for i in range(count.value):items[i]=C.pointer(objects[i]);put(0x429748+i*4,0x500000+i*72)
    actions={i:(rng.randrange(2),8+i if rng.randrange(2) else -1,rng.randrange(2),30 if i==0 and case%3==0 else -1) for i in range(8)}
    actions.update({i:(0,-1,False,-1) for i in range(8,32)})
    release_append=case%4==0
    put(0x4289f0,count.value);put(0x4218c8,ticks.value);u.reg_write(UC_X86_REG_ESP,0x800000);ce.clear();ne.clear()
    u.emu_start(0x40229b,0x4022f2,count=100000)
    lib.sniper_projectile_frame(items,C.byref(count),C.byref(ticks),step,release,None)
    assert ce==ne,(case,ce,ne)
    assert count.value==get(0x4289f0) and ticks.value==get(0x4218c8),case
    for i in range(32):
        expected=get(0x429748+i*4);got=items[i].contents.word[0] if items[i] else None
        assert got==(get(expected) if expected else None),(case,i,got,expected)
    appended_visits+=sum(e[0]=='step' and e[1]>=8 for e in ce)
assert appended_visits>0
print(f'700 original frame loops match live list/count/ticks,release order and stale tails;{appended_visits} same-frame appended visits')
