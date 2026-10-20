"""Compare generated tables/RNG and full indexed buffers with original x86."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32)
u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
STACK,PIXELS,OBJECT,VTABLE,LOCK,UNLOCK,STOP=0x1000000,0x1100000,0x1300000,0x1301000,0x1302000,0x1303000,0x1304000
u.mem_map(STACK,65536);u.mem_map(PIXELS,0x200000);u.mem_map(OBJECT,0x5000)
def word(addr,value):u.mem_write(addr,struct.pack('<I',value&0xffffffff))
word(OBJECT,VTABLE);word(VTABLE+0x64,LOCK);word(VTABLE+0x80,UNLOCK);word(0x42931c,OBJECT)
lib=C.CDLL(str(ROOT/'code/build/libstarfield.so'))
class Star(C.Structure):_fields_=[('x',C.c_int16),('y',C.c_int16)]
class Field(C.Structure):_fields_=[('near',Star*3969),('far',Star*3969)]
lib.sniper_starfield_generate.argtypes=[C.POINTER(Field),C.POINTER(C.c_uint32)]
lib.sniper_starfield_draw.argtypes=[C.POINTER(Field),C.POINTER(C.c_uint8)]+[C.c_int32]*5+[C.c_uint8]*2
field=Field();rng=random.Random(87)
for seed in [0,1,87,0xffffffff]+[rng.getrandbits(32) for _ in range(16)]:
    state=C.c_uint32(seed);word(0x422f78,seed)
    u.reg_write(UC_X86_REG_EBP,STACK+32768);u.reg_write(UC_X86_REG_ESP,STACK+32000)
    u.emu_start(0x403569,0x403612,count=2000000)
    assert u.reg_read(UC_X86_REG_EIP)==0x403612
    lib.sniper_starfield_generate(C.byref(field),C.byref(state))
    assert bytes(field.near)==bytes(u.mem_read(0x424358,3969*4)),seed
    assert bytes(field.far)==bytes(u.mem_read(0x429f68,3969*4)),seed
    assert state.value==struct.unpack('<I',u.mem_read(0x422f78,4))[0],seed
print('20 complete starfield generations and final RNG states match original x86')
pitch=width=height=0;calls=[]
def intercept(uc,addr,size,user):
    if addr not in (LOCK,UNLOCK):return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if addr==LOCK:
        ret,obj,rect,desc,flags,event=struct.unpack('<6I',uc.mem_read(sp,24))
        assert (obj,rect,flags,event)==(OBJECT,0,1,0)
        word(desc+16,pitch);word(desc+36,PIXELS)
        uc.reg_write(UC_X86_REG_ESP,sp+24);calls.append('lock')
    else:
        ret,obj,ptr=struct.unpack('<3I',uc.mem_read(sp,12))
        assert (obj,ptr)==(OBJECT,PIXELS)
        uc.reg_write(UC_X86_REG_ESP,sp+12);calls.append('unlock')
    uc.reg_write(UC_X86_REG_EAX,0);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,intercept,begin=LOCK,end=UNLOCK)
cases=changed=0
for width,height in [(1,1),(319,201),(640,480),(801,601),(1024,768)]:
    pitch=width+13
    for cx,cy in [(-100,-100),(0,0),(319,320),(8100,8200),(15000,15000)]+[(rng.randrange(15000),rng.randrange(15000)) for _ in range(25)]:
        initial=bytes([197])*(pitch*height+32);buffer=(C.c_uint8*len(initial)).from_buffer_copy(initial)
        near_color,far_color=rng.randrange(1,127),rng.randrange(128,196)
        result=lib.sniper_starfield_draw(C.byref(field),buffer,pitch,width,height,cx,cy,near_color,far_color)
        if result==-1:continue # explicit safe rejection outside generated tables
        u.mem_write(PIXELS,initial)
        for addr,value in [(0x42818c,width),(0x42434c,height),(0x42de80,cx),(0x424324,cy),(0x4289e8,near_color),(0x42834c,far_color)]:word(addr,value)
        calls.clear();u.reg_write(UC_X86_REG_ESP,STACK+32768);word(STACK+32768,STOP)
        u.emu_start(0x409840,STOP,count=2000000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP
        assert calls==['lock','unlock']
        expected=bytes(u.mem_read(PIXELS,len(initial)))
        assert bytes(buffer)==expected,(cx,cy,width,height)
        changed+=sum(a!=197 for a in expected);cases+=1
assert changed>1000
# Invalid table range must leave caller pixels untouched.
buffer=(C.c_uint8*100)(*([197]*100))
assert lib.sniper_starfield_draw(C.byref(field),buffer,10,10,10,-10000,-10000,1,2)==-1
assert bytes(buffer)==bytes([197])*100
print(f'{cases} indexed starfield buffers match original x86; {changed} painted pixels; padding preserved')
