"""404680 and411540, actual pickup constructor; controlled damage/detach/allocator."""
import ctypes as C
import pathlib,struct,random,sys
REAL_DAMAGE="--real-damage" in sys.argv
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,128)).split(b'\0')[0].decode()
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
class Pickups(C.Structure):_fields_=[('items',C.POINTER(C.POINTER(Pickup))),('count',C.c_int32),('map',C.c_void_p),('local',C.POINTER(Ship)),('enabled',C.c_int32),('limit',C.c_int32),('lookup',C.c_void_p),('release',C.c_void_p),('award',C.c_void_p),('user',C.c_void_p)]
class Notices(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
Allocate=C.CFUNCTYPE(C.c_void_p,C.c_void_p)
Damage=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,C.c_int32)
Detach=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
class World(C.Structure):_fields_=[('players',C.POINTER(C.POINTER(Ship))),('local',C.POINTER(Ship)),('local_id',C.c_int32),('bounty',C.c_int32),('lifetime',C.c_int32),('pickups',C.POINTER(Pickups)),('notices',C.POINTER(Notices)),('allocate',Allocate),('damage',Damage),('detach',Detach),('user',C.c_void_p)]
ships=(Ship*3)();ptrs=(C.POINTER(Ship)*1024)();items=(C.POINTER(Pickup)*16)();pickup=Pickup();grid=(C.c_uint8*(1024*1024))();hud=Notices()
pw=Pickups(items,0,C.addressof(grid),C.pointer(ships[0]),0,0,None,None,None,None)
ce=[];ne=[];fail=False
@Allocate
def allocate(_):ce.append(('allocate',20));return None if fail else C.addressof(pickup)
@Damage
def damage(_,s,amount,owner):
    if REAL_DAMAGE:damage_lib.sniper_ship_damage(s,C.byref(dw),amount,owner)
    else:ce.append(('damage',s.contents.word[17],amount,owner));s.contents.word[23]=-1;s.contents.word[24]=102
@Detach
def detach(_,target):ce.append(('detach',target));ships[0].word[27]=target
w=World(ptrs,C.pointer(ships[0]),0,7,6000,C.pointer(pw),C.pointer(hud),allocate,damage,detach,None)
lib=C.CDLL(str(ROOT/'code/build/libdeath_event.so'));lib.sniper_death_event.argtypes=[C.POINTER(World)]+[C.c_int32]*4;lib.sniper_offline_death.argtypes=[C.POINTER(World),C.c_int32,C.c_int32]
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32)
@Sound
def sound(_,x,y):ce.append(('sound',x,y))
class DamageWorld(C.Structure):_fields_=[('config',C.c_void_p),('map',C.c_void_p),('local',C.c_int32),('random',C.c_void_p),('sound',Sound),('death',C.c_void_p),('message',C.c_void_p),('user',C.c_void_p)]
if REAL_DAMAGE:
    damage_lib=C.CDLL(str(ROOT/'code/build/libship_damage.so'));damage_lib.sniper_ship_damage.argtypes=[C.POINTER(Ship),C.POINTER(DamageWorld),C.c_int32,C.c_int32]
    dw=DamageWorld(None,None,0,None,sound,None,None,None)
def hook(uc,a,size,data):
    if REAL_DAMAGE and a==0x414b10:return
    if a not in [0x416b20,0x414b10,0x520200,0x416bc0,0x407a80]:return
    sp=uc.reg_read(UC_X86_REG_ESP);obj=uc.reg_read(UC_X86_REG_ECX);clean=0;value=0
    if a==0x407a80:ne.append(('sound',get(sp+8),get(sp+12)))
    elif a==0x416b20:ne.append(('allocate',get(sp+4)));value=0 if fail else 0x530000
    elif a==0x414b10:
        ne.append(('damage',get(obj+0x44),get(sp+4),get(sp+8)));put(obj+0x5c,-1);put(obj+0x60,102);clean=8
    elif a==0x520200:ne.append(('detach',get(sp+4)));put(0x50006c,get(sp+4));clean=4
    else:
        fmt=string(get(sp+8));args=[string(get(sp+12)),get(sp+16)]
        if fmt.count('%s')==2:args.append(string(get(sp+20)))
        text=(fmt%tuple(args)).encode();uc.mem_write(get(sp+4),text+b'\0');value=len(text)
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
for a,v in [(0x429f48,0x600000),(0x4218e0,0x500000),(0x429318,0),(0x42826d,7),(0x428211,6000),(0x424348,0x520000),(0x520000,0x520100),(0x52012c,0x520200)]:put(a,v)
rng=random.Random(0x404680);seen=set();created=0
for case in range(900):
    C.memset(ships,0,C.sizeof(ships));C.memset(grid,0,C.sizeof(grid));C.memset(items,0,C.sizeof(items));C.memset(ptrs,0,C.sizeof(ptrs))
    for i in range(3):
        s=ships[i];name=f'Pilot{i}'.encode()+b'\0';C.memmove(C.byref(s),name,len(name))
        for off,val in [(0x44,i),(0x48,rng.choice([400000,-1,-16000,16384000])),(0x4c,400000),(0x50,rng.choice([0,1])),(0x54,rng.choice([0,-1])),(0x6c,rng.choice([-1,0,1,2,3])),(0x78,17),(0x7c,3),(0x80,rng.choice([0,100,2147483647])),(0xa0,100)]:s.word[off//4]=val
        u.mem_write(0x500000+i*536,bytes(s));ptrs[i]=C.pointer(s)
    for i in range(4):put(0x429338+i*4,0x500000+i*536 if i<3 else 0)
    C.memset(C.byref(hud),0x41,C.sizeof(hud))
    for i in range(4):hud.ticks[i]=rng.choice([-1,0,1,50]);hud.color[i]=rng.randrange(4)
    u.mem_write(0x424330,bytes(hud.ticks));u.mem_write(0x4287e8,bytes(hud.text));u.mem_write(0x428a50,bytes(hud.color))
    grid[25*1024+25]=rng.choice([0,0,152,255]);grid[25*1024]=grid[25*1024+25];u.mem_write(0x600000,bytes(grid))
    pw.count=rng.choice([0,15,16]);put(0x428a08,pw.count);u.mem_write(0x429288,bytes(64));C.memset(C.byref(pickup),0x55,20);u.mem_write(0x530000,bytes(pickup))
    fail=case%11==0;ce.clear();ne.clear();killer=case%4;victim=(case//4)%4;points=rng.choice([-1000,0,50,2147483647]);prize=rng.choice([-1,0,7,21])
    offline=case%5==0
    args=[killer,points] if offline else [killer,victim,points,prize]
    put(0x800000,0x810000)
    for i,v in enumerate(args):put(0x800004+4*i,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x411540 if offline else 0x404680,0x810000,count=100000)
    if offline:lib.sniper_offline_death(C.byref(w),killer,points)
    else:lib.sniper_death_event(C.byref(w),killer,victim,points,prize)
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),(case,'ships')
    assert bytes(hud.ticks)==bytes(u.mem_read(0x424330,16)),(case,'ticks')
    assert bytes(hud.text)==bytes(u.mem_read(0x4287e8,512)),(case,'text')
    assert bytes(hud.color)==bytes(u.mem_read(0x428a50,16)),(case,'color')
    assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),(case,'map')
    assert bytes(pickup)==bytes(u.mem_read(0x530000,20)),(case,'pickup')
    assert pw.count==get(0x428a08),(case,'count')
    for i in range(16):assert bool(items[i])==bool(get(0x429288+4*i)),(case,'list',i)
    assert ce==ne,(case,ce,ne)
    seen.update(e[0] for e in ce);created+=int(bool(ce) and ce[0][0]=='allocate' and not fail)
assert seen==({'allocate','sound','detach'} if REAL_DAMAGE else {'allocate','damage','detach'}),seen
assert created>0
print(('Actual remote death: ' if REAL_DAMAGE else '')+f'900 death events/offline dispatches match ships,HUD,map,pickups,list and effects;{created} actual pickup constructions')
