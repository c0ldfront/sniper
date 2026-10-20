"""Compare sprite draw parameters and animation calculations with original x86."""
import ctypes as C
import pathlib
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
pe=pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32)
u.mem_map(0x400000,0x200000);u.mem_write(0x400000,pe.get_memory_mapped_image())
u.mem_map(0x1000000,0x10000)
OBJ,STACK,SURFACE=0x1000000,0x1008000,0x1001000
u.mem_write(0x42f848,struct.pack('<I',SURFACE))
u.mem_write(SURFACE,struct.pack('<I',SURFACE+256))
class Sprite(C.Structure):
    _fields_=[(s,C.c_int32) for s in ['resource','ox','oy','w','h','rows','cols','period','frames']]
class Rect(C.Structure):
    _fields_=[(s,C.c_int32) for s in ['left','top','right','bottom']]
class Blit(C.Structure):
    _fields_=[('source',Rect),('destination',Rect),('flags',C.c_uint32)]
lib=C.CDLL(str(ROOT/'code/build/libsprite.so'))
lib.sniper_sprite_blit.argtypes=[C.POINTER(Sprite),C.c_int32,C.c_int32,C.c_int32,C.c_int,C.c_int,C.POINTER(Blit)]
lib.sniper_sprite_frame.argtypes=[C.POINTER(Sprite),C.c_int32,C.POINTER(C.c_int32)]
sprites=list((Sprite*4).in_dll(lib,'sniper_large_tiles'))
# Test the separate single-row path and odd-sized centering as well.
sprites.append(Sprite(0,8,9,17,13,1,7,35,7))
draws=0;animations=0
for s in sprites:
    u.mem_write(OBJ,struct.pack('<I',SURFACE+512))
    u.mem_write(OBJ+0x84,struct.pack('<iiiiii',s.w,s.h,s.rows,s.cols,s.period,s.frames))
    for frame in [-1,0,1,s.cols-1,s.cols,s.frames-1,s.frames]:
        for x,y in [(-21,-11),(0,0),(320,240)]:
            for centered in [0,1]:
                for opaque in [0,1]:
                    u.reg_write(UC_X86_REG_ECX,OBJ);u.reg_write(UC_X86_REG_ESP,STACK)
                    u.mem_write(STACK,struct.pack('<Iiiiii',0,x,y,frame,centered,opaque))
                    u.emu_start(0x4105c0,0x41069d,count=1000)
                    sp=u.reg_read(UC_X86_REG_ESP)
                    surface,dest,srcsurface,src,flags,fx=struct.unpack('<6I',u.mem_read(sp,24))
                    expected_src=struct.unpack('<4i',u.mem_read(src,16))
                    expected_dst=struct.unpack('<4i',u.mem_read(dest,16))
                    b=Blit();assert lib.sniper_sprite_blit(C.byref(s),x,y,frame,centered,opaque,C.byref(b))==0
                    assert tuple(getattr(b.source,k) for k,_ in Rect._fields_)==expected_src
                    assert tuple(getattr(b.destination,k) for k,_ in Rect._fields_)==expected_dst
                    assert b.flags==flags
                    draws+=1
    for tick in [-(2**31),-241,-1,*range(0,s.period*2+1),2**31-1]:
        u.reg_write(UC_X86_REG_ECX,OBJ);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_EAX,tick&0xffffffff)
        u.emu_start(0x406c71,0x406c96,count=100)
        expected=C.c_int32(u.reg_read(UC_X86_REG_EAX)).value
        frame=C.c_int32();assert lib.sniper_sprite_frame(C.byref(s),tick,C.byref(frame))==0
        assert frame.value==expected,(s.period,tick,frame.value,expected)
        animations+=1
print(f'{draws} sprite blit parameter comparisons and {animations} animation comparisons passed')
