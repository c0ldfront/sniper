"""Compare ordered draw arguments from original full particle renderer."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(ROOT/'code/build/libfont.so'))
Draw=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int32,C.c_int32,C.c_int32)
lib.sniper_font_draw.argtypes=[C.c_char_p,C.c_int32,C.c_int32,C.c_int,Draw,C.c_void_p]
lib.sniper_font_draw.restype=C.c_int32
lib.sniper_font_width.argtypes=[C.c_char_p];lib.sniper_font_width.restype=C.c_int32
native=[];recovered=[]
@Draw
def draw(_,resource,x,y,frame):recovered.append((resource,x,y,frame))
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def hook(uc,addr,size,data):
    if addr!=0x4105c0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    assert get(sp+16)==0 and get(sp+20)==0
    native.append((uc.reg_read(UC_X86_REG_ECX),*[C.c_int32(get(sp+i)).value for i in (4,8,12)]))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+24)
u.hook_add(UC_HOOK_CODE,hook)

for i in range(4):put(0x4282f8+i*4,i)
rng=random.Random(87)
cases=[b'',b'Pilot,Squad(100)',bytes(range(1,256))]
cases += [bytes(rng.randrange(1,256) for _ in range(rng.randrange(100))) for _ in range(300)]
for text in cases:
    color=rng.randrange(4);x=rng.choice([-2147483648,2147483640,10]);y=-10
    u.mem_write(0x500000,text+b'\0')
    for j,v in enumerate([0x520000,0x500000,x,y,color]):put(0x510000+4*j,v)
    u.reg_write(UC_X86_REG_ESP,0x510000)
    native.clear();recovered.clear();u.emu_start(0x415d60,0x520000,count=100000)
    width=lib.sniper_font_draw(text,x,y,color,draw,None)
    assert native==recovered
    assert width==C.c_int32(u.reg_read(UC_X86_REG_EAX)).value
    u.reg_write(UC_X86_REG_ESP,0x510000)
    u.emu_start(0x415dd0,0x520000,count=100000)
    assert lib.sniper_font_width(text)==C.c_int32(u.reg_read(UC_X86_REG_EAX)).value
print('303 font strings match original glyph draws and both width results, including all nonzero byte values')
# Opaque panel glyph calls ignore every individual blit failure, including
# negative-x titles wider than body-derived panel width (spectator stats).
glyphs=[];failure=0
def opaque_hook(uc,addr,size,data):
    if addr!=0x4104a0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    assert get(sp+4)==0x530000 and get(sp+20)==0 and get(sp+24)==1
    glyphs.append((C.c_int32(get(sp+8)).value,get(sp+12),get(sp+16)))
    uc.reg_write(UC_X86_REG_EAX,failure)
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+28)
u.hook_add(UC_HOOK_CODE,opaque_hook)
text=b' ^   1 ~    Points';u.mem_write(0x500000,text+b'\0')
for failure in [0,0x88760096,0x887601c2,0x80004005]:
    for j,v in enumerate([0x520000,0x530000,0x500000,-6,3,3]):put(0x510000+4*j,v)
    glyphs.clear();u.reg_write(UC_X86_REG_ESP,0x510000);u.emu_start(0x416090,0x520000,count=10000)
    assert glyphs==[(-6+6*i,3,ch-32) for i,ch in enumerate(text)]
    assert u.reg_read(UC_X86_REG_EAX)==len(text)*6
print('PASS opaque panel text ignores individual glyph HRESULTs and continues all title glyphs')
