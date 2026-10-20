"""Full native dynamic radar stage; only final rectangle fill is intercepted."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
class View(C.Structure):_fields_=[(n,C.c_int32) for n in ['x','y','size','dx','dy']]
Rect=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_int32,C.c_int32,C.c_uint32)
class Markers(C.Structure):
    _fields_=[('local',C.POINTER(Ship)),('reference',C.POINTER(Ship)),('ships',C.POINTER(C.POINTER(Ship))),('pickups',C.POINTER(C.POINTER(Pickup))),('projectiles',C.POINTER(C.POINTER(Projectile)))]+[(n,C.c_int32) for n in ['ship_count','pickup_count','projectile_count','local_id','watched_id','screen_width','map_width','map_height','tick','special','tile_x','tile_y','upper','lower','bonus']]+[(n,C.c_uint32) for n in ['friendly','positive','nonpositive','projectile_color','pickup_color']]+[('view',View),('rect',Rect),('user',C.c_void_p)]
lib=C.CDLL(str(root/'code/build/libradar_markers.so'));lib.sniper_radar_markers.argtypes=[C.POINTER(Markers)]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
ce=[];ne=[];colors=set()
@Rect
def rect(_,l,t,r,b,color):ce.append((l,t,r,b,color))
def hook(uc,a,size,user):
    if a==0x40769e:uc.emu_stop();return
    if a!=0x415e50:return
    sp=uc.reg_read(UC_X86_REG_ESP);p=get(sp+8);ne.append(tuple(get(p+i*4) for i in range(4))+(get(sp+12),))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87);draw_count=0
for case in range(1800):
    ships=(Ship*8)();pickups=(Pickup*8)();projectiles=(Projectile*12)()
    for i,s in enumerate(ships):
        s.word[0x44//4]=i;s.word[0x40//4]=r.choice([0,1,3,4]);s.word[0x48//4]=r.choice([-1,r.randrange(-8500000,12000000)])
        s.word[0x4c//4]=r.randrange(-8500000,12000000);s.word[0xa4//4]=r.choice([0,1,2]);s.word[0x68//4]=r.randrange(3)
        s.word[0x6c//4]=r.choice([-1,-1,-1,2,3]);s.word[0xa0//4]=r.choice([-2147483648,-100,0,1,100,2147483647])
        s.word[0x88//4]=r.randrange(2);s.word[0x94//4]=r.randrange(2)
        s.word[0x5c//4]=r.choice([-1,0,1000000])
        put(0x4283e8+4*i,0x500000+536*i);put(0x429338+4*i,0x500000+536*i)
    for i,p in enumerate(pickups):p.x=r.randrange(-100,800);p.y=r.randrange(-100,800);put(0x429288+4*i,0x530000+20*i)
    for i,p in enumerate(projectiles):
        p.word[1]=r.randrange(-8500000,12000000);p.word[2]=r.randrange(-8500000,12000000)
        p.word[6]=r.choice([-1,0,1,5]);p.word[8]=r.randrange(3);p.word[13]=r.randrange(1,7);put(0x429748+4*i,0x550000+72*i)
    sptrs=(C.POINTER(Ship)*8)(*[C.pointer(s) for s in ships]);pptrs=(C.POINTER(Pickup)*8)(*[C.pointer(p) for p in pickups]);qptrs=(C.POINTER(Projectile)*12)(*[C.pointer(p) for p in projectiles])
    watched=r.choice([-1,0,3,7]);reference=C.pointer(ships[0])
    if ships[0].word[0x40//4]==4:
        reference=C.pointer(ships[watched]) if watched>=0 else None
    if case%11==0 and ships[0].word[0x40//4]==4 and watched>=0:reference=None;put(0x429338+4*watched,0)
    put(0x429334,0) # original ID-indexed lookup for watched -1
    m=Markers(C.pointer(ships[0]),reference,sptrs,pptrs,qptrs,8,8,12,0,watched,r.choice([320,640,1024]),r.choice([160,320,512]),r.choice([160,320,512]),r.choice([-2147483648,-33,-17,-8,0,8,16,24,32,2147483647]),case%9==0,r.randrange(1024),r.randrange(1024),r.choice([0,10,100,2147483647]),r.choice([-10,0,10,100]),r.choice([-100,0,10,100,2147483647]),11,6,7,8,10,View(r.randrange(80),r.randrange(80),r.choice([52,104,168]),500,300),rect,None)
    u.mem_write(0x500000,bytes(ships));u.mem_write(0x530000,bytes(pickups));u.mem_write(0x550000,bytes(projectiles))
    for a,v in [(0x4218e0,0x500000),(0x429318,0),(0x424340,watched),(0x429328,8),(0x428a08,8),(0x4289f0,12),(0x42818c,m.screen_width),(0x4218cc,m.tick),(0x4218d0,m.special),(0x4218d4,m.tile_x),(0x4218d8,m.tile_y),(0x4282bd,m.upper),(0x4282c1,m.lower),(0x4282c5,m.bonus),(0x424328,11),(0x428340,6),(0x42932c,7),(0x4281a8,8),(0x424350,10),(0x42931c,0x580000)]:put(a,v)
    for off,v in [(0x1c,m.map_width),(0x20,m.map_height),(0x14,m.view.x),(0x24,m.view.y),(0x10,m.view.size),(0x2c,m.view.dx),(0x30,m.view.dy)]:put(0x800000+off,v)
    ce.clear();ne.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4075d9,0x407a70,count=100000)
    lib.sniper_radar_markers(C.byref(m))
    assert ce==ne,(case,ce,ne)
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),case
    colors.update(v[-1] for v in ce);draw_count+=len(ce)
assert colors=={6,7,8,10,11},colors
print(f'PASS1800 full original dynamic-radar cases:{draw_count} matching rectangles; all colors,visibility/blink/crop,special/spectator,ships,pickups and projectiles')
