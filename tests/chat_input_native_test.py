"""Original editor path, including macro consumption and submission timing."""
import ctypes as C,pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class State(C.Structure):_fields_=[('text',C.c_char*82),('macros',(C.c_char*82)*8),('enabled',C.c_int32),('dirty',C.c_int32)]
CB=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p)
lib=C.CDLL(str(root/'code/build/libchat_input.so'))
lib.sniper_chat_input.argtypes=[C.POINTER(State),C.c_int32,C.c_uint8,C.c_int32,C.c_int32,CB,C.c_void_p]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,82)).split(b'\0')[0]
native=[];expected=[];shift=0
@CB
def submit(user,text):expected.append(text)
def hook(uc,a,size,user):
    if a not in (0x700000,0x409b10):return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x700000:
        assert get(sp+4)==16
        uc.reg_write(UC_X86_REG_EAX,shift);clean=4
    else:native.append(string(get(sp+4)));clean=0
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook);put(0x4354c0,0x700000)
r=random.Random(87)
for i in range(2400):
    width=r.choice([320,640,800]);ch=r.randrange(256);key=r.choice([0,8,13,0x74]+list(range(0x70,0x78)))
    shift=r.choice([0,0x8000]);key=key if shift or key in [0,8,13,0x74] else 0
    text=bytes(r.choice(b" abcXYZ123/'=") for _ in range(r.choice([0,1,42,43,75,76,80])))
    s=State();s.text=text;s.enabled=r.randrange(2);s.dirty=r.randrange(2)
    u.mem_write(0x428358,text+b'\0'+bytes(81-len(text)))
    for j in range(8):
        macro=('macro%d'%j).encode() if r.randrange(2) else b''
        s.macros[j].value=macro;u.mem_write(0x42e2a8+82*j,macro+b'\0'+bytes(81-len(macro)))
    put(0x42818c,width);u.mem_write(0x422244,bytes([ch]));put(0x422248,key);put(0x42224c,123)
    put(0x42819c,s.enabled);put(0x428188,s.dirty)
    native.clear();expected.clear();u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x40526d,0x405d56,count=100000)
    consumed=lib.sniper_chat_input(C.byref(s),width,ch,key,shift,submit,None)
    assert s.text==string(0x428358),(i,key,shift,s.text,string(0x428358))
    assert expected==native,(i,expected,native)
    assert s.enabled==get(0x42819c) and s.dirty==get(0x428188)
    assert [x.value for x in s.macros]==[string(0x42e2a8+82*j) for j in range(8)]
    assert get(0x422248)==(0 if consumed else key)
    assert get(0x42224c)==(0 if consumed else 123)
print('PASS2400 original chat editor cases:ASCII/width limits,backspace,Enter gate,F5,8 macros and key/repeat consumption')
