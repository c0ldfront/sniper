"""406332..406431, exact input priority,key polling and wrapping cooldown."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_EBP,UC_X86_REG_EDI
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
Key=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_int32)
class Input(C.Structure):_fields_=[(n,C.c_int32) for n in ['joy','buttons','pov','mouse','x','y','extent']]+[('key',Key),('user',C.c_void_p)]
class Selection(C.Structure):_fields_=[('active',C.c_int32),('action',C.c_int32)]
ce=[];ne=[];keys={}
@Key
def key(_,vk):ce.append(vk);return keys[vk]
lib=C.CDLL(str(ROOT/'code/build/libweapon_input.so'));lib.sniper_weapon_input.argtypes=[C.POINTER(Input),C.c_int32,C.POINTER(C.c_int32)];lib.sniper_weapon_input.restype=Selection
put(0x4354b8,0x530000)
def hook(uc,a,size,data):
    if a!=0x530000:return
    sp=uc.reg_read(UC_X86_REG_ESP);vk=get(sp+4);ne.append(vk)
    uc.reg_write(UC_X86_REG_EAX,keys[vk]&0xffffffff);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x406332);seen=set()
for case in range(2400):
    keys={vk:rng.choice([0,1,0x8000,-32768]) for vk in [9,16,17]}
    inp=Input(case%2,rng.randrange(16),rng.choice([-1,0,9000,18000,27000,4500]),case%3,rng.choice([-1,0,59,60,61]),rng.choice([0,320,321,327,328,352,353,375,383,384,2147483647]),rng.choice([640,641,-3,2147483647,-2147483648]),key,None)
    cooldown=C.c_int32(rng.choice([-2147483648,-1,0,25,2147483647]));dt=rng.choice([0,1,50,2147483647])
    for a,v in [(0x4218c8,dt),(0x429300,cooldown.value),(0x422278,inp.joy),(0x4292e8,inp.buttons),(0x4292f0,inp.pov),(0x800014,inp.mouse),(0x42229c,inp.x),(0x4222a0,inp.y),(0x42434c,inp.extent)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);ce.clear();ne.clear();u.emu_start(0x406332,0x406431,count=10000)
    result=lib.sniper_weapon_input(C.byref(inp),dt,C.byref(cooldown))
    native_action=C.c_int32(u.reg_read(UC_X86_REG_EDI)).value
    assert (result.active,result.action,cooldown.value)==(u.reg_read(UC_X86_REG_EBP),native_action,get(0x429300)),case
    assert ce==ne,(case,ce,ne)
    seen.add(result.action)
assert set(range(8))<=seen,seen
print('2400 native input-selection blocks match action/active,cooldown overflow and ordered key polls across all8 actions')
