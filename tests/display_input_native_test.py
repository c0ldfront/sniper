"""Native Shift precedence and ordinary display keys through405d56."""
import ctypes as C,pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(root/'code/build/libdisplay_input.so'));lib.sniper_display_input.argtypes=[C.c_int32,C.POINTER(C.c_int32),C.POINTER(C.c_int32),C.POINTER(C.c_int32)]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
macros=[]
def hook(uc,a,size,user):
    if a not in (0x700000,0x409b10):return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x700000:assert get(sp+4)==16;uc.reg_write(UC_X86_REG_EAX,shift&0xffffffff);extra=4
    else:macros.append(get(sp+4));extra=0
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+extra)
u.hook_add(UC_HOOK_CODE,hook);put(0x4354c0,0x700000)
r=random.Random(40591)
for case in range(1200):
    key=r.choice([0,0x72,0x75,0x7a]);shift=r.choice([0,1,0x8000,-32768]);mode=C.c_int32(r.choice([-2147483648,-4,-1,0,1,2,3,2147483647]));stars=C.c_int32(r.choice([-1,0,1,2]));ticker=C.c_int32(r.choice([-1,0,1,2]));repeat=r.randrange(-2147483648,2147483648)
    for a,v in [(0x422248,key),(0x42224c,repeat),(0x428344,mode.value),(0x42e018,stars.value),(0x422284,ticker.value),(0x4282e1,0)]:put(a,v)
    u.mem_write(0x428358,b'\0');u.mem_write(0x42e2a8,bytes(656));macros.clear()
    u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4052cd,0x405d56,count=10000)
    consumed=bool(shift&0x8000 and key in (0x72,0x75,0x7a))
    portable_key=0 if consumed else key
    lib.sniper_display_input(portable_key,C.byref(mode),C.byref(stars),C.byref(ticker))
    assert (mode.value,stars.value,ticker.value)==(get(0x428344),get(0x42e018),get(0x422284)),case
    assert get(0x422248)==portable_key and get(0x42224c)==(0 if consumed else repeat),case
    assert bool(macros)==bool(shift&0x8000 and key in (0x72,0x75)),case
print('PASS1200 native display key paths: F3 signed wrapped cycle,F6 stars,F11 ticker; Shift macros/voice precedence and key/repeat retention')
