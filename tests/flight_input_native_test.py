"""Compare keyboard/joystick steering block with original x86, mocking key reads."""
import ctypes as C
import pathlib,struct,random,itertools
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ESI
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
SHIP=0x500000;STACK=0x510000;KEY=0x520000
class Control(C.Structure):_fields_=[(n,C.c_int32) for n in ['heading','rotation_rate','lock_ticks','attached_id']]
class Input(C.Structure):_fields_=[(n,C.c_uint32) for n in ['keys','joystick_enabled','joystick_x','joystick_y']]
lib=C.CDLL(str(ROOT/'code/build/libflight_input.so'))
lib.sniper_flight_input.argtypes=[C.POINTER(Control),C.POINTER(Input),C.c_int32]
lib.sniper_flight_input.restype=C.c_int32
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
put(0x4354b8,KEY);put(0x4218e0,SHIP)
key_bits={0x26:1,0x28:2,0x25:4,0x27:8,0x10:16}
keys=0
def hook(uc,addr,size,data):
    if addr!=KEY:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    uc.reg_write(UC_X86_REG_EAX,0x8000 if keys & key_bits[get(sp+4)] else 0)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(87);count=0
cases=[]
for keys,enabled,locked,attached in itertools.product(range(32),range(2),[0,1],[-1,4]):
    for axis in [0,0x3fff,0x4000,0x5556,0x8000,0xaaaa,0xc000,0xc001,0xffff]:
        cases.append((keys,enabled,locked,attached,axis,axis,rng.randrange(40000),rng.choice([33,66,99]),rng.randrange(51)))
for _ in range(1000):
    cases.append((rng.randrange(32),1,0,-1,rng.getrandbits(32),rng.getrandbits(32),rng.randrange(40000),rng.randrange(200),rng.randrange(51)))
for keys,enabled,locked,attached,jx,jy,heading,rate,ticks in cases:
    for a,v in [(SHIP+0x58,heading),(SHIP+0xcc,rate),(SHIP+0x104,locked),(SHIP+0x6c,attached),(0x422278,enabled),(0x4292d0,jx),(0x4292d4,jy),(0x4218c8,ticks)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,STACK)
    u.emu_start(0x405ffa,0x40610d,count=300)
    expect_heading=C.c_int32(get(SHIP+0x58)).value
    expect_thrust=C.c_int32(u.reg_read(UC_X86_REG_ESI)).value
    state=Control(heading,rate,locked,attached);inp=Input(keys,enabled,jx,jy)
    thrust=lib.sniper_flight_input(C.byref(state),C.byref(inp),ticks)
    assert (state.heading,thrust)==(expect_heading,expect_thrust),(keys,jx,jy,state.heading,expect_heading,thrust,expect_thrust)
    count+=1
print(f'{count} keyboard/joystick heading and thrust selections match original x86')
