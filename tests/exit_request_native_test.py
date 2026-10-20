"""Compare actual Escape branch405564..405d56 including all notice writes."""
import ctypes as C
import pathlib, random, struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Notices(C.Structure):
    _fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
lib=C.CDLL(str(root/'code/build/libexit_request.so'))
lib.sniper_exit_request.argtypes=[C.POINTER(C.c_int32),C.POINTER(C.c_int32),C.POINTER(Notices)]
def put(a,x):u.mem_write(a,struct.pack('<i',x))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
r=random.Random(87)
for case in range(600):
    mode=C.c_int32(r.choice([0,0,0,1,2,3,4,-1]));pending=C.c_int32(r.choice([0,1,-7]))
    n=Notices();C.memset(C.addressof(n),0x5a,C.sizeof(n))
    for i in range(4):n.ticks[i]=r.choice([-2147483648,-1,0,1,1500,2147483647]);n.color[i]=r.randrange(-5,6)
    put(0x4282ec,mode.value);put(0x428160,pending.value)
    u.mem_write(0x424330,bytes(n.ticks));u.mem_write(0x4287e8,bytes(n.text));u.mem_write(0x428a50,bytes(n.color))
    u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x405564,0x405d56,count=10000)
    lib.sniper_exit_request(C.byref(mode),C.byref(pending),C.byref(n))
    assert mode.value==get(0x4282ec) and pending.value==get(0x428160),case
    assert bytes(n.ticks)==bytes(u.mem_read(0x424330,16)),case
    assert bytes(n.text)==bytes(u.mem_read(0x4287e8,512)),case
    assert bytes(n.color)==bytes(u.mem_read(0x428a50,16)),case
print('PASS600 original Escape branch comparisons: overlay, pending, notice slots/text/timers/colors')
