"""Radar map size, camera crop and actual million-tile raster against native."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class View(C.Structure):_fields_=[(n,C.c_int32) for n in ['x','y','size','dx','dy']]
lib=C.CDLL(str(root/'code/build/libradar_map.so'))
lib.sniper_radar_map_size.argtypes=[C.c_int32,C.c_int32,C.c_int,C.POINTER(C.c_int32)]
lib.sniper_radar_view.argtypes=[C.c_int32]*7+[C.POINTER(View)]
lib.sniper_radar_map_pixels.argtypes=[C.c_void_p,C.c_size_t,C.c_int32,C.c_int32,C.c_int32,C.c_void_p,C.c_uint8,C.c_uint8]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
r=random.Random(87)
for case in range(1600):
    width=r.choice([320,640,641,1024,-1,2147483647,-2147483648]);height=r.choice([240,480,768]);scale=r.choice([1,2,8,16,32,100,-7]);small=case%2
    put(0x42818c,width);put(0x428279,scale);put(0x42def4,small);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x407247,0x40726f,count=100);size=C.c_int32()
    assert lib.sniper_radar_map_size(width,scale,small,C.byref(size))==1
    assert size.value==get(0x80004c),case
    x=r.choice([r.randrange(-20000,20000),2147483647,-2147483648]);y=r.randrange(-20000,20000);mw=r.choice([80,160,320,512,1024]);mh=r.choice([80,320,512])
    for a,v in [(0x42818c,width),(0x42434c,height),(0x42de80,x),(0x424324,y),(0x42def4,small),(0x80001c,mw),(0x800020,mh)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4073c7,0x4074b0,count=1000)
    view=View();lib.sniper_radar_view(width,height,x,y,mw,mh,small,C.byref(view))
    assert [view.x,view.y,view.size,view.dx,view.dy]==[get(0x800000+o) for o in [0x14,0x24,0x10,0x2c,0x30]],case
print('PASS1600 original radar size/camera-crop cases including wrap and signed bounds',flush=True)
for case,(width,height,pad) in enumerate([(80,80,0),(160,160,4),(320,320,0),(240,240,8),(81,79,3),(512,256,0),(40,40,0),(320,320,16)]):
    grid=bytearray(1048576)
    for i in range(case+1,len(grid),97):grid[i]=r.choice([0,1,151,152,153,250,251,255])
    for i in range(1024):grid[i]=1;grid[-1-i]=152;grid[i*1024]=250;grid[i*1024+1023]=151
    pitch=width+pad;size=(height+1)*pitch+width+1
    data=(C.c_uint8*size)(*([73]*size));native=bytes(data);map_data=(C.c_uint8*1048576).from_buffer(grid)
    u.mem_write(0x600000,bytes(grid));u.mem_write(0x700000,native)
    for a,v in [(0x429f48,0x600000),(0x428338,22),(0x424320,11),(0x800064,0x700000),(0x800050,pitch),(0x80001c,width),(0x800020,height)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x40732d,0x4073b3,count=40000000)
    assert lib.sniper_radar_map_pixels(data,size,pitch,width,height,map_data,11,22)==1
    assert bytes(data)==bytes(u.mem_read(0x700000,size)),case
    print(f'PASS original1024x1024 radar raster {case+1}/8: {width}x{height},pitch{pitch},including padded edge writes',flush=True)
