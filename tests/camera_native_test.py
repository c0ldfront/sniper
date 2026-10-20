"""Full original camera selection and shake,with real CRT RNG."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2];u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Camera(C.Structure):_fields_=[('x',C.c_int32),('y',C.c_int32)]
lib=C.CDLL(str(ROOT/'code/build/libcamera.so'))
lib.sniper_camera.argtypes=[C.POINTER(Ship),C.POINTER(Ship)]+[C.c_int32]*6+[C.POINTER(C.c_uint32)];lib.sniper_camera.restype=Camera
rng=random.Random(402338)
for case in range(1600):
    local=Ship();watched=Ship();local.word[0x40//4]=case%5
    for ship in [local,watched]:
        for o in [0x48,0x4c]:ship.word[o//4]=rng.choice([-1,0,999,-999,16383999,C.c_int32(rng.getrandbits(32)).value])
    width=rng.choice([640,800,1024,1920,-1,-0x80000000,0x7fffffff]);height=rng.choice([480,600,768,1080,-1,-0x80000000,0x7fffffff])
    special=case%3==0;present=case%2==0;tx=C.c_int32(rng.getrandbits(32)).value;ty=C.c_int32(rng.getrandbits(32)).value
    shake=rng.choice([-1,0,1,3,4,50,300,0x7fffffff]);seed=rng.getrandbits(32)
    for a,v in [(0x4218d0,special),(0x4218d4,tx),(0x4218d8,ty),(0x42818c,width),(0x42434c,height),
        (0x429738,shake),(0x4218e0,0x500000),(0x424340,2),(0x429340,0x501000 if present else 0),(0x422f78,seed)]:put(a,v)
    u.mem_write(0x500000,bytes(local));u.mem_write(0x501000,bytes(watched));u.reg_write(UC_X86_REG_ESP,0x510000)
    u.emu_start(0x402338,0x402468,count=10000);assert u.reg_read(UC_X86_REG_EIP)==0x402468
    state=C.c_uint32(seed);result=lib.sniper_camera(C.byref(local),C.byref(watched) if present else None,width,height,special,tx,ty,shake,C.byref(state))
    assert (result.x&0xffffffff,result.y&0xffffffff)==(get(0x42de80),get(0x424324)),case
    assert state.value==get(0x422f78),case
print('1600 camera cases match original position and shared RNG state')
