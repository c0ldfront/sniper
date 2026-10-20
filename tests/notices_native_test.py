"""Native notice insertion,frame aging and display layout in sequence."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2];u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Notices(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32,C.c_int32)
ce=[];ne=[]
@Draw
def draw(_,text,x,y,color):ce.append((text,x,y,color))
def hook(uc,a,size,data):
    if a!=0x415cf0:return
    sp=uc.reg_read(UC_X86_REG_ESP);text=bytes(uc.mem_read(get(sp+8),128)).split(b'\0',1)[0]
    ne.append((text,get(sp+12),get(sp+16),get(sp+20)))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
lib=C.CDLL(str(ROOT/'code/build/libnotices.so'))
lib.sniper_notice_push.argtypes=[C.POINTER(Notices),C.c_char_p,C.c_int32,C.c_int32]
lib.sniper_notices_step.argtypes=[C.POINTER(Notices),C.c_int32]
lib.sniper_notices_draw.argtypes=[C.POINTER(Notices),C.c_int32,C.c_int32,Draw,C.c_void_p]
n=Notices();C.memset(C.byref(n),0x5a,C.sizeof(n));rng=random.Random(407)
for i in range(4):n.ticks[i]=0
u.mem_write(0x424330,bytes(n.ticks));u.mem_write(0x4287e8,bytes(n.text));u.mem_write(0x428a50,bytes(n.color))
for case in range(400):
    text=('Notice'+str(case)+'x'*rng.randrange(100)).encode();duration=rng.choice([-1,0,1,50,300,600]);color=rng.randrange(4)
    u.mem_write(0x500000,text+b'\0')
    for i,v in enumerate([0x520000,0x500000,duration,color]):put(0x510000+4*i,v)
    u.reg_write(UC_X86_REG_ESP,0x510000);u.emu_start(0x407b50,0x520000,count=10000)
    lib.sniper_notice_push(C.byref(n),text,duration,color)
    dt=rng.choice([0,1,50,300,0x7fffffff]);put(0x4218c8,dt)
    u.emu_start(0x4022f2,0x402310,count=1000);lib.sniper_notices_step(C.byref(n),dt)
    assert bytes(n.ticks)==bytes(u.mem_read(0x424330,16));assert bytes(n.text)==bytes(u.mem_read(0x4287e8,512));assert bytes(n.color)==bytes(u.mem_read(0x428a50,16))
    width=rng.choice([640,800,1024,0x7fffffff,-0x80000000]);height=rng.choice([480,600,768,0x7fffffff,-0x80000000])
    put(0x42818c,width);put(0x42434c,height);u.reg_write(UC_X86_REG_ESP,0x510000);ce.clear();ne.clear()
    u.emu_start(0x4025c9,0x402626,count=10000);lib.sniper_notices_draw(C.byref(n),width,height,draw,None)
    assert ce==ne,(case,ce,ne)
print('400 sequential notice insertion/aging/layout cases match all slot bytes and draw calls')
