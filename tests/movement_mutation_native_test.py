"""Original movement with synchronous pickup mutations of the moving ship."""
import ctypes as C
import pathlib,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['x','y','value']]
Pick=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_int32,C.c_int32,C.c_int)
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32)
class World(C.Structure):
    _fields_=[('map',C.POINTER(C.c_uint8))]+[(n,C.c_int32) for n in ['local_id','watched_id','camera_x','camera_y','bounce_divisor']]+[('pickups',C.POINTER(Pickup)),('pickup_count',C.c_int32),('pickup',Pick),('impact_sound',Sound),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libmovement.so'));lib.sniper_move_object.argtypes=[C.POINTER(C.c_int32),C.POINTER(World),C.c_int32,C.c_int]
grid=(C.c_uint8*(1024*1024))();ship=(C.c_int32*134)();items=[]
for y in range(23,28):
    for x in range(23,28):items.append((x,y,7))
picks=(Pickup*len(items))(*(Pickup(*p) for p in items));cevents=[];nevents=[];mode=0
# Apply the same external effect to emulator and recovered world.
def mutate(native,x,y):
    def setf(offset,value):
        if native:put(0x500000+offset,value)
        else:ship[offset//4]=value
    if mode==1:setf(0x60,10)
    elif mode==2:setf(0x64,10)
    elif mode==3:setf(0x6c,2)
    elif mode==4:setf(0x44,3)
    elif mode==5:setf(0x50,-2000);setf(0x54,3000)
    elif mode==6:setf(0x48,420000);setf(0x4c,380000)
    elif mode==7:setf(0x48,-1)
    elif mode==8:
        if native:u.mem_write(0x600000+y*1024+x,b'\0')
        else:grid[y*1024+x]=0
    elif mode==9:
        if native:put(0x428a08,0)
        else:world.pickup_count=0
@Pick
def pick(_,x,y,player,value,local):
    cevents.append(('pick',x,y,player,value if local else 0,local));mutate(False,x,y)
@Sound
def sound(_,x,y):cevents.append(('sound',x,y))
world=World(grid,1,2,0,0,32,picks,len(items),pick,sound,None)
def hook(uc,a,size,data):
    if a not in [0x404910,0x407a80,0x820100]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x407a80:nevents.append(('sound',get(sp+8),get(sp+12)))
    else:
        x,y=get(sp+4),get(sp+8)
        if a==0x820100:player,value,local=get(0x500044),get(sp+12),1;clean=12
        else:player,value,local=get(sp+12),0,0
        nevents.append(('pick',x,y,player,value,local));mutate(True,x,y)
    uc.reg_write(UC_X86_REG_EIP,get(sp)&0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
for a,v in [(0x429f48,0x600000),(0x424348,0x820000),(0x820000,0x820040),(0x82005c,0x820100),
            (0x429318,1),(0x424340,2),(0x42de80,0),(0x424324,0),(0x428271,32)]:put(a,v)
for i,(x,y,value) in enumerate(items):
    ptr=0x830000+i*32;put(0x429288+4*i,ptr)
    for off,v in [(4,x),(8,y),(16,value)]:put(ptr+off,v)
count=0
for mode in range(10):
    for vx,vy in [(2000,0),(-2000,0),(0,2000),(0,-2000),(2000,-2000)]:
        for dt in [1,2,10]:
            for player in [1,3]:
                C.memset(grid,0,len(grid))
                for x,y,_ in items:grid[y*1024+x]=255
                # Adjacent solid tiles exercise collision after callback mutation.
                for y in range(22,29):grid[y*1024+22]=1;grid[y*1024+28]=1
                C.memset(ship,0,C.sizeof(ship))
                for off,v in [(0x44,player),(0x48,400000),(0x4c,400000),(0x50,vx),(0x54,vy),(0x6c,-1)]:ship[off//4]=v
                u.mem_write(0x500000,bytes(ship));u.mem_write(0x600000,bytes(grid));put(0x428a08,len(items));world.pickup_count=len(items)
                for i,v in enumerate([0x810000,dt,0x500048,0x50004c,0x500050,0x500054,1]):put(0x800000+4*i,v)
                cevents.clear();nevents.clear();u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000)
                u.emu_start(0x414450,0x810000,count=100000)
                lib.sniper_move_object(ship,C.byref(world),dt,1)
                assert bytes(ship)==bytes(u.mem_read(0x500000,536)),(mode,vx,vy,dt,player,'ship')
                assert cevents==nevents,(mode,vx,vy,dt,player,cevents,nevents)
                assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),(mode,'map')
                assert world.pickup_count==get(0x428a08)
                assert any(e[0]=='pick' for e in cevents)
                count+=1
print(f'{count} synchronous pickup-mutation cases match full ship, map and ordered effects')
