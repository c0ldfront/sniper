"""Actual Insert branch and complete native spawn; sound/clock API seams only."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Notices(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
lib=C.CDLL(str(root/'code/build/libmanual_warp_test.so'))
lib.test_manual_warp.argtypes=[C.POINTER(Ship),C.c_void_p,C.POINTER(Notices),C.POINTER(C.c_int32)]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
grid=(C.c_uint8*1048576)()
for y in range(0,1024,19):
    for x in range(1024):grid[y*1024+x]=1
u.mem_write(0x600000,bytes(grid))
config=(C.c_int32*72)();lib.sniper_local_config(config);u.mem_write(0x4281c9,bytes(config))
for a,v in [(0x429f48,0x600000),(0x4218e0,0x500000),(0x4218e8,87),(0x4353f4,0x530000)]:put(a,v)
ms=0;sounds=[];spawned=False
def hook(uc,a,size,user):
    global spawned
    if a==0x4137c0:spawned=True
    if a not in [0x530000,0x40aae0]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x530000:uc.reg_write(UC_X86_REG_EAX,ms)
    else:
        assert uc.reg_read(UC_X86_REG_ECX)==87
        assert [get(sp+i) for i in [4,8,12,16]]==[0,0,0,0]
        sounds.append(get(0x50005c));clean=16
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(600):
    ship=Ship();n=Notices()
    for i in range(134):ship.word[i]=r.randrange(-2147483648,2147483648)
    ship.word[0x40//4]=case%4
    capacity=r.choice([-1,0,1,500000,1000000,2147483647]);ship.word[0xc8//4]=capacity
    ship.word[0x5c//4]=capacity if case%2==0 else capacity^1
    ms=r.getrandbits(32);values=(C.c_int32*7)(C.c_int32(ms).value,C.c_int32(r.getrandbits(32)).value,-7,r.randrange(1,1025),0,0,0)
    C.memset(C.addressof(n),0x5a,C.sizeof(n))
    for i in range(4):n.ticks[i]=r.choice([-1,0,1,1500])
    u.mem_write(0x500000,bytes(ship));u.mem_write(0x424330,bytes(n.ticks));u.mem_write(0x4287e8,bytes(n.text));u.mem_write(0x428a50,bytes(n.color))
    for a,v in [(0x422f78,values[1]),(0x4292fc,values[2]),(0x429328,values[3])]:put(a,v)
    sounds.clear();spawned=False;u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x40577a,0x405d56,count=2000000)
    lib.test_manual_warp(C.byref(ship),grid,C.byref(n),values)
    assert bytes(ship)==bytes(u.mem_read(0x500000,C.sizeof(ship))),case
    assert values[1]==get(0x422f78) and values[2]==get(0x4292fc),case
    assert values[4]==len(sounds) and values[5]==(sounds[0] if sounds else 0),case
    assert bool(values[6])==spawned==(case%2==0),case
    assert bytes(n.ticks)==bytes(u.mem_read(0x424330,16)),case
    assert bytes(n.text)==bytes(u.mem_read(0x4287e8,512)),case
    assert bytes(n.color)==bytes(u.mem_read(0x428a50,16)),case
print('PASS600 actual Insert/warp cases: native spawn, full ship, map rejection, RNG, notices, dirty and pre-warp sound order')
