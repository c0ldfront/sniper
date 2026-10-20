"""Ordinary local control gates,joystick acquisition,stage order and cleanup."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
STACK,SHIP,JOY=0x1000000,0x1100000,0x1200000
for a in (STACK,SHIP,JOY):u.mem_map(a,65536)
def word(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def words(a,n):return struct.unpack('<'+'I'*n,u.mem_read(a,4*n))
word(0x4218e0,SHIP);word(0x435580,JOY)
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Damage=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,C.c_int32)
Poll=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
Stage=C.CFUNCTYPE(None,C.c_void_p,C.c_int)
class Control(C.Structure):_fields_=[('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ('key','repeat','mx','my','joystick_enabled','local_id')]+[('character',C.c_uint8),('joystick',C.c_uint32*13),('damage',Damage),('poll',Poll),('stage',Stage),('user',C.c_void_p)]
actual=[];native=[];polled=[];mutated_energy=-1;status=0
@Damage
def damage(_,s,amount,owner):actual.append(('damage',amount,owner));s.contents.word[0x5c//4]=mutated_energy
@Poll
def poll(_,idx,info):
    actual.append(('poll',idx,tuple(info[i] for i in range(13))))
    for i in range(13):info[i]=polled[i]
    return status
@Stage
def stage(_,i):actual.append(('stage',i))
def intercept(uc,a,size,user):
    if a in (0x405f97,0x406a9f):uc.emu_stop();return
    stages={0x405ffa:(0,0x406332),0x406332:(1,0x406922),0x406922:(2,0x406a84)}
    if a in stages:
        i,next_=stages[a];native.append(('stage',i));uc.reg_write(UC_X86_REG_EIP,next_);return
    if a not in (JOY,0x414b10):return
    sp=uc.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0]
    if a==JOY:
        idx,ptr=words(sp+4,2);native.append(('poll',idx,words(ptr,13)));uc.mem_write(ptr,struct.pack('<13I',*polled));uc.reg_write(UC_X86_REG_EAX,status)
    else:
        amount,owner=words(sp+4,2);assert uc.reg_read(UC_X86_REG_ECX)==SHIP
        native.append(('damage',C.c_int32(amount).value,C.c_int32(owner).value));word(SHIP+0x5c,mutated_energy)
    uc.reg_write(UC_X86_REG_ESP,sp+12);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,intercept)
lib=C.CDLL(str(ROOT/'code/build/liblocal_control.so'));lib.sniper_local_control.argtypes=[C.POINTER(Control)]
rng=random.Random(87);deaths=0
for case in range(700):
    ship=Ship();energy=rng.choice([-1,0,1000000]);ship.word[0x5c//4]=energy
    c=Control(C.pointer(ship),rng.choice([0,123,65]),rng.randrange(10),17,-5,rng.randrange(2),rng.randrange(1024),65,(C.c_uint32*13)(*([0x12345678]*13)),damage,poll,stage,None)
    polled=[rng.getrandbits(32) for _ in range(13)];mutated_energy=rng.choice([-1,0,100]);status=rng.choice([0,165,167])
    u.mem_write(SHIP,bytes(ship));u.mem_write(0x4292c8,bytes(c.joystick))
    for a,v in [(0x422248,c.key),(0x42224c,c.repeat),(0x4222a4,c.mx),(0x4222a8,c.my),(0x422278,c.joystick_enabled),(0x429318,c.local_id)]:word(a,v)
    u.mem_write(0x422244,bytes([c.character]));actual.clear();native.clear();u.reg_write(UC_X86_REG_ESP,STACK+32768)
    u.emu_start(0x405f75,0x406aa6,count=100000)
    assert u.reg_read(UC_X86_REG_EIP) in (0x405f97,0x406a9f)
    result=lib.sniper_local_control(C.byref(c));assert result==int(energy>=0);deaths+=not result
    assert actual==native,(case,actual,native)
    assert list(c.joystick)==list(words(0x4292c8,13))
    assert c.key==C.c_int32(words(0x422248,1)[0]).value and c.repeat==C.c_int32(words(0x42224c,1)[0]).value
    assert c.mx==C.c_int32(words(0x4222a4,1)[0]).value and c.my==C.c_int32(words(0x4222a8,1)[0]).value
    assert c.character==u.mem_read(0x422244,1)[0] and bytes(ship)==bytes(u.mem_read(SHIP,536))
print(f'700 local-control lifecycle traces/state comparisons pass;{deaths} initially dead cases;stage bodies intercepted')
