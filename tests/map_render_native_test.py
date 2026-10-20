"""Compare complete tile draw sequences with original x86 map-render loop."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
p=pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image())
MAP,STACK,DESC,STOP=0x1000000,0x1200000,0x1300000,0x1400000
for address,size in [(MAP,1048576),(STACK,65536),(DESC,65536),(STOP,4096)]:u.mem_map(address,size)
u.mem_write(0x429f48,struct.pack('<I',MAP));u.mem_write(0x4281b4,struct.pack('<I',DESC))
for i,(period,frames) in enumerate([(150,30),(150,30),(240,30),(120,15)]):
    addr=DESC+(i+1)*256
    u.mem_write(0x42da18+(216+i)*4,struct.pack('<I',addr))
    u.mem_write(addr+0x94,struct.pack('<ii',period,frames))
native=[]
def intercept(uc,address,size,data):
    sp=uc.reg_read(UC_X86_REG_ESP)
    ret,x,y,frame,centered,opaque=struct.unpack('<Iiiiii',uc.mem_read(sp,24))
    obj=uc.reg_read(UC_X86_REG_ECX)
    tile=frame+1 if obj==DESC else 215+(obj-DESC)//256
    assert centered==0
    native.append((tile,x,y,frame,opaque))
    uc.reg_write(UC_X86_REG_ESP,sp+24);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,intercept,begin=0x4105c0,end=0x4105c0)
CB=C.CFUNCTYPE(None,C.c_void_p,C.c_uint,C.c_int32,C.c_int32,C.c_int32,C.c_int)
lib=C.CDLL(str(ROOT/'code/build/librender.so'))
lib.sniper_map_draw.argtypes=[C.POINTER(C.c_uint8),C.c_int32,C.c_int32,C.c_int32,C.c_int32,C.c_int32,CB,C.c_void_p]
rng=random.Random(87)
grid=bytearray(1048576)
for i in range(len(grid)):
    grid[i]=rng.choice([0,1,20,152,153,215,216,217,218,219,250,251])
u.mem_write(MAP,bytes(grid));cgrid=(C.c_uint8*len(grid)).from_buffer_copy(grid)
cases=0
for cx,cy in [(-100,-100),(-1,-1),(0,0),(15,16),(16,15),(127,255),(8100,8200),(16300,16300),(16384,16384),(17000,17000)]:
    for w,h in [(320,200),(640,480),(1024,768)]:
        for tick in [0,149,240]:
            native.clear();actual=[]
            for addr,val in [(0x42de80,cx),(0x424324,cy),(0x42818c,w),(0x42434c,h),(0x4218cc,tick)]:u.mem_write(addr,struct.pack('<i',val))
            u.reg_write(UC_X86_REG_ESP,STACK+32768);u.mem_write(STACK+32768,struct.pack('<I',STOP))
            u.emu_start(0x406b70,STOP,count=2000000)
            cb=CB(lambda _,tile,x,y,f,o:actual.append((tile,x,y,f,o)))
            assert lib.sniper_map_draw(cgrid,cx,cy,w,h,tick,cb,None)==0
            assert native==actual,(cx,cy,w,h,tick,len(native),len(actual))
            cases+=1
print(f'{cases} complete map-render draw sequences match original x86 0x406b70')
