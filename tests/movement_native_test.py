"""Execute original full movement function, intercepting only external effects."""
import ctypes as C
import pathlib, struct, random
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32)
u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
SHIP=0x500000; MAP=0x600000; STACK=0x800000; RETURN=0x810000; NET=0x820000
class Motion(C.Structure): _fields_=[(n,C.c_int32) for n in ['x','y','vx','vy']]
class Ship(C.Structure): _fields_=[(n,C.c_int32) for n in ['player_id','death_ticks','respawn_ticks','attached_id']]+[('ship_x',C.POINTER(C.c_int32))]
class Pickup(C.Structure): _fields_=[(n,C.c_int32) for n in ['x','y','value']]
Pick=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_int32,C.c_int32,C.c_int)
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32)
class World(C.Structure):
    _fields_=[('map',C.POINTER(C.c_uint8))]+[(n,C.c_int32) for n in ['local_id','watched_id','camera_x','camera_y','bounce_divisor']]+[('pickups',C.POINTER(Pickup)),('pickup_count',C.c_int32),('pickup',Pick),('impact_sound',Sound),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libmovement.so'))
lib.sniper_move.argtypes=[C.POINTER(Motion),C.POINTER(Ship),C.POINTER(World),C.c_int32,C.c_int]
lib.sniper_move.restype=None
def put(a,v): u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a): return struct.unpack('<i',u.mem_read(a,4))[0]
native_events=[]; c_events=[]
@Pick
def pickup(_,x,y,player,value,local): c_events.append(('pick',x,y,player,value if local else 0,local))
@Sound
def sound(_,x,y): c_events.append(('sound',x,y))
def hook(uc,addr,size,data):
    if addr not in (0x404910,0x407a80,NET+0x100): return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if addr==0x407a80:
        native_events.append(('sound',get(sp+8),get(sp+12)))
        clean=0
    elif addr==0x404910:
        native_events.append(('pick',get(sp+4),get(sp+8),get(sp+12),0,0));clean=0
    else:
        native_events.append(('pick',get(sp+4),get(sp+8),get(SHIP+0x44),get(sp+12),1));clean=12
    ret=get(sp)&0xffffffff
    uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
    uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,hook)
put(0x429f48,MAP);put(0x424348,NET);put(NET,NET+0x40);put(NET+0x40+0x1c,NET+0x100)
rng=random.Random(414450);count=0
effect_counts={"sound":0,"local_pickup":0,"remote_pickup":0}
# Structured walls, corners, pass-through tiles and pickups in a small arena.
grid=bytearray(1024*1024)
for y in range(20,31):
    for x in range(20,31):
        if x in (20,30) or y in (20,30): grid[y*1024+x]=rng.choice([1,20,152,216,250,255])
grid[25*1024+25]=255
items=[(25,25,7),(20,25,9),(30,25,4)]
for x,y,_ in items:grid[y*1024+x]=255
u.mem_write(MAP,bytes(grid));c_grid=(C.c_uint8*len(grid)).from_buffer_copy(grid)
picks=(Pickup*len(items))(*(Pickup(*p) for p in items))
put(0x428a08,len(items))
for i,(x,y,v) in enumerate(items):
    ptr=0x830000+i*32;put(0x429288+4*i,ptr)
    put(ptr+4,x);put(ptr+8,y);put(ptr+16,v)
cases=[]
for axis in range(2):
    for sign in (-1,1):
        for speed in (0,1,999,1000,1001,5000):
            for ticks in (0,1,2,5,50):
                xy=[400000,400000];vel=[0,0];vel[axis]=sign*speed
                cases.append((*xy,*vel,ticks))
for _ in range(1500):
    cases.append((rng.randrange(300000,510000),rng.randrange(300000,510000),rng.randrange(-6000,6001),rng.randrange(-6000,6001),rng.randrange(51)))
# Outer map bounds and signed-division behavior near zero.
for x in (0,1,13999,14000,16370000,16383999):
    for y in (0,14000,16383999):
        for vx,vy in ((-4000,2000),(4000,-2000),(0,0)):
            cases.append((x,y,vx,vy,8))
for index,(x,y,vx,vy,ticks) in enumerate(cases):
    player=rng.choice([1,2,3]);watch=rng.choice([2,4]);camera=rng.choice([0,100,1000])
    divisor=rng.choice([16,17,24,32]);effects=rng.choice([0,1])
    death=rng.choice([0,0,1]);respawn=rng.choice([0,0,1]);attached=rng.choice([-1,-1,2])
    u.mem_write(SHIP,bytes(0x200))
    for off,v in [(0x44,player),(0x48,x),(0x4c,y),(0x50,vx),(0x54,vy),(0x60,death),(0x64,respawn),(0x6c,attached)]:put(SHIP+off,v)
    for a,v in [(0x429318,1),(0x424340,watch),(0x42de80,camera),(0x424324,camera),(0x428271,divisor)]:put(a,v)
    for i,v in enumerate([RETURN,ticks,SHIP+0x48,SHIP+0x4c,SHIP+0x50,SHIP+0x54,effects]):put(STACK+4*i,v)
    u.reg_write(UC_X86_REG_ECX,SHIP);u.reg_write(UC_X86_REG_ESP,STACK)
    native_events.clear();c_events.clear()
    u.emu_start(0x414450,RETURN,count=100000)
    assert u.reg_read(UC_X86_REG_EIP)==RETURN
    expected=[get(SHIP+off) for off in (0x48,0x4c,0x50,0x54)]
    m=Motion(x,y,vx,vy)
    ship=Ship(player,death,respawn,attached,C.cast(C.byref(m),C.POINTER(C.c_int32)))
    world=World(c_grid,1,watch,camera,camera,divisor,picks,len(items),pickup,sound,None)
    lib.sniper_move(C.byref(m),C.byref(ship),C.byref(world),ticks,effects)
    actual=[m.x,m.y,m.vx,m.vy]
    assert actual==expected,(index,(x,y,vx,vy,ticks),actual,expected)
    assert c_events==native_events,(index,c_events,native_events)
    for event in native_events:
        key="sound" if event[0]=="sound" else "local_pickup" if event[-1] else "remote_pickup"
        effect_counts[key]+=1
    count+=1
assert all(effect_counts.values()), effect_counts
print(effect_counts)
print(f'{count} full movement calls match original x86 positions, velocities and effect-call sequences')
