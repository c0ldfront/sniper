"""413510 comparison; allocation/constructor, sound and damage are seams."""
import ctypes as C
import pathlib,struct,random,sys
REAL_DAMAGE="--real-damage" in sys.argv
REAL_CREATE="--real-create" in sys.argv
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32)
Damage=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,C.c_int32)
Create=C.CFUNCTYPE(C.c_void_p,C.c_void_p,C.POINTER(C.c_int32))
class World(C.Structure):
    _fields_=[('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ['radius','maximum','duration','shake','speed','group','count']]+[('projectiles',C.POINTER(C.POINTER(Projectile))),('sound',Sound),('damage',Damage),('create',Create),('user',C.c_void_p)]
ce=[];ne=[];alloc=0;fail=False;children=[]
@Sound
def sound(_,x,y):ce.append(('sound',x,y))
@Damage
def damage(_,s,amount,owner):
    if REAL_DAMAGE:damage_lib.sniper_ship_damage(s,C.byref(dw),amount,owner)
    else:ce.append(('damage',amount,owner));s.contents.word[0x5c//4]-=amount
@Create
def create(_,args):
    ce.append(('allocate',72))
    if fail:return None
    ce.append(('create',tuple(args[i] for i in range(15))))
    if REAL_CREATE:
        child=Projectile();C.memset(C.byref(child),0x55,72)
        create_lib.sniper_projectile_create(C.byref(child),None,args);children.append(child)
    return 0x550000+sum(e[0]=='create' for e in ce)*72
lib=C.CDLL(str(ROOT/'code/build/libprojectile_explosion.so'))
lib.sniper_projectile_explode.argtypes=[C.POINTER(Projectile),C.POINTER(World)]
if REAL_CREATE:
    create_lib=C.CDLL(str(ROOT/'code/build/libprojectile_create.so'))
    create_lib.sniper_projectile_create.argtypes=[C.POINTER(Projectile),C.c_void_p,C.POINTER(C.c_int32)]
class Config(C.Structure):_fields_=[('word',C.c_int32*72)]
Random=C.CFUNCTYPE(C.c_int32,C.c_void_p)
Message=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32)
class DamageWorld(C.Structure):_fields_=[('config',C.POINTER(Config)),('map',C.c_void_p),('local',C.c_int32),('random',Random),('sound',Sound),('death',Sound),('message',Message),('user',C.c_void_p)]
@Random
def random_value(_):ce.append(('random',0));return 0
@Message
def message(_,text,duration,color):ce.append(('message',text.decode(),duration,color))
@Sound
def death(_,owner,bounty):ce.append(('death',owner,bounty))
if REAL_DAMAGE:
    damage_lib=C.CDLL(str(ROOT/'code/build/libship_damage.so'))
    damage_lib.sniper_ship_damage.argtypes=[C.POINTER(Ship),C.POINTER(DamageWorld),C.c_int32,C.c_int32]
    config=Config();damage_lib.sniper_local_config(C.byref(config));grid=(C.c_uint8*(1024*1024))()
    dw=DamageWorld(C.pointer(config),C.addressof(grid),1,random_value,sound,death,message,None)
    u.mem_write(0x4281c9,bytes(config));put(0x429f48,0x600000);u.mem_write(0x600000,bytes(grid))
    put(0x429318,1);put(0x424348,0x520000);put(0x520000,0x520100);put(0x520114,0x520200)
def hook(uc,a,size,data):
    global alloc
    if REAL_DAMAGE and a==0x414b10:return
    if a not in [0x407a80,0x414b10,0x416b20,0x412b60,0x4165f0,0x407b50,0x520200]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;value=0
    if a==0x4165f0:ne.append(('random',0))
    elif a==0x407b50:ne.append(('message',bytes(u.mem_read(get(sp+4),100)).split(b'\0')[0].decode(),get(sp+8),get(sp+12)))
    elif a==0x520200:ne.append(('death',get(sp+4),get(sp+8)));clean=8
    elif a==0x407a80:ne.append(('sound',get(sp+8),get(sp+12)))
    elif a==0x414b10:
        amount,owner=get(sp+4),get(sp+8);ne.append(('damage',amount,owner));put(0x51005c,get(0x51005c)-amount);clean=8
    elif a==0x416b20:
        ne.append(('allocate',get(sp+4)))
        if not fail:
            alloc+=1;value=0x550000+alloc*72
            if REAL_CREATE:u.mem_write(value,bytes([0x55])*72)
    else:
        ne.append(('create',tuple(get(sp+4+i*4) for i in range(15))))
        if REAL_CREATE:return
        clean=60;value=uc.reg_read(UC_X86_REG_ECX)
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(413510);seen=set()
for case in range(800):
    p=Projectile();s=Ship();ptrs=(C.POINTER(Projectile)*512)()
    for j in range(18):p.word[j]=rng.randint(-100,100)
    for off,val in [(0,12),(4,8000000),(8,8000000),(0x28,7999000),(0x2c,8001000),(0x38,case%4),(0x44,rng.choice([0,1,2,7,16])),(0x3c,case%3),(0x40,case%4)]:p.word[off//4]=val
    s.word[0x48//4]=8000000+rng.choice([0,999,1000,30000,120000,500000]);s.word[0x4c//4]=8000000+rng.randrange(-200000,200001);s.word[0x5c//4]=1000000
    if REAL_DAMAGE:
        s.word[0x5c//4]=[1,10000,1000000][case%3];s.word[0x44//4]=1
        s.word[0xc4//4]=config.word[(0x428251-0x4281c9)//4]+10000
        s.word[0x108//4]=case%7==0
    w=World(C.pointer(s),rng.choice([30,60,120]),rng.choice([1000,200000]),40,7,2500,123,rng.choice([0,490,500,511]),ptrs,sound,damage,create,None)
    fail=case%13==0;alloc=0;ce.clear();ne.clear();children.clear()
    u.mem_write(0x500000,bytes(p));u.mem_write(0x510000,bytes(s));u.mem_write(0x429748,bytes(2048))
    for a,v in [(0x4218e0,0x510000),(0x42820d,w.radius),(0x4281dd,w.maximum),(0x428215,w.duration),(0x429738,w.shake),(0x4281f1,w.speed),(0x428178,w.group),(0x4289f0,w.count),(0x800000,0x810000)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000)
    u.emu_start(0x413510,0x810000,count=100000)
    lib.sniper_projectile_explode(C.byref(p),C.byref(w))
    assert bytes(p)==bytes(u.mem_read(0x500000,72)),case
    assert bytes(s)==bytes(u.mem_read(0x510000,536)),case
    assert ce==ne,(case,ce,ne)
    for i,child in enumerate(children):assert bytes(child)==bytes(u.mem_read(0x550000+(i+1)*72,72)),(case,'child',i)
    assert (w.shake,w.group,w.count)==tuple(get(a) for a in [0x429738,0x428178,0x4289f0]),case
    for i in range(w.count):assert C.cast(ptrs[i],C.c_void_p).value==(get(0x429748+i*4) or None),(case,i)
    seen.update(e[0] for e in ce)
assert seen==({'sound','random','message','death','allocate','create'} if REAL_DAMAGE else {'sound','damage','allocate','create'}),seen
print(('Actual shrapnel construction: ' if REAL_CREATE else '')+('Integrated actual ship damage: ' if REAL_DAMAGE else '')+'800 full explosion comparisons pass: projectile/local state, shake, list, group IDs and ordered constructor/damage/sound calls')
