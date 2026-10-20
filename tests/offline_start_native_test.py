"""Actual post-backend startup,sort,spawn and spectator selection."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ESI
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
lib=C.CDLL(str(ROOT/'code/build/liboffline_start_test.so'))
lib.test_offline_start.argtypes=[C.POINTER(Ship),C.POINTER(C.c_int32),C.c_void_p,C.POINTER(C.c_int32)]
config=(C.c_int32*72)();lib.sniper_local_config(config)
grid=(C.c_uint8*1048576)()
for a,v in [(0x429f48,0x600000),(0x429328,12),(0x4353f4,0x530000),(0x4354cc,0x530100),(0x424348,0x520000),(0x520000,0x520100),(0x520120,0x4115d0),(0x423f98,0)]:put(a,v)
seen=set();cursor_calls=0
def hook(uc,a,size,user):
    global cursor_calls
    if a in (0x4137c0,0x4115d0):seen.add(a)
    if a not in (0x530000,0x530100):return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x530000:uc.reg_write(UC_X86_REG_EAX,milliseconds)
    else:
        assert (get(sp+4),get(sp+8))==(0,0);cursor_calls+=1;clean=8
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x403703)
for case in range(600):
    ships=(Ship*12)()
    for i,s in enumerate(ships):
        C.memset(C.byref(s),0x55,536)
        name=rng.choice([b'alpha',b'ALPHA',b'Beta',b'z']);C.memmove(C.byref(s),name+b'\0',len(name)+1)
        s.word[0x44//4]=i;s.word[0x68//4]=rng.randrange(4)
        s.word[0x40//4]=4 if case%7==0 else rng.randrange(6)
        put(0x429338+4*i,0x500000+536*i)
    ships[0].word[0x40//4]=case%6
    order=list(range(12));rng.shuffle(order);order=(C.c_int32*12)(*order)
    milliseconds=rng.getrandbits(32)
    values=(C.c_int32*12)(C.c_int32(milliseconds).value,rng.randrange(2**31),-7,rng.choice([-24,0,240,480,768]),rng.choice([-1,0,4,10,32]),case%2,case%16,0,0,0,0,0)
    u.mem_write(0x500000,bytes(ships));u.mem_write(0x600000,bytes(grid));u.mem_write(0x4281c9,bytes(config))
    for i,j in enumerate(order):put(0x4283e8+i*4,0x500000+j*536)
    for a,v in [(0x422f78,values[1]),(0x4292fc,values[2]),(0x42434c,values[3]),(0x42e538,values[4]),(0x422280,values[5]),(0x429f58,values[6])]:put(a,v)
    cursor_calls=0;u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EAX,0);u.reg_write(UC_X86_REG_ESI,0xffffffff)
    u.emu_start(0x403703,0x40383b,count=2000000)
    lib.test_offline_start(ships,order,grid,values)
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),case
    assert list(order)==[(get(0x4283e8+4*i)-0x500000)//536 for i in range(12)],case
    for i,a in [(1,0x422f78),(2,0x4292fc),(4,0x42e538),(6,0x429f58),(7,0x424340),(8,0x42def0),(9,0x4289f4),(10,0x42e140)]:assert values[i]==get(a),(case,i,values[i],get(a))
    assert values[11]==cursor_calls==1
    assert get(0x4218e0)==0x500000 and get(0x429318)==0
assert {0x4137c0,0x4115d0}<=seen,seen
print('600 startup completions match native ship bytes,sort,local/frequency/watch selection,spawn RNG,dirty flags,history and cursor request;actual spawn/sort')
