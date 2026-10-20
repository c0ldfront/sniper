"""Full413010 with controlled explosion,damage,RNG and positional sound effects."""
import ctypes as C
import pathlib,struct,random,sys
FRAME="--frame" in sys.argv
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2];u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Explode=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Projectile));Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32)
Random=C.CFUNCTYPE(C.c_int32,C.c_void_p);Damage=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,C.c_int32)
class World(C.Structure):
    _fields_=[('map',C.c_void_p),('coverage',C.c_void_p),('stride',C.c_int32),('ships',C.POINTER(C.POINTER(Ship))),('ship_count',C.c_int32),
      ('projectiles',C.POINTER(C.POINTER(Projectile))),('projectile_count',C.c_int32)]+[(n,C.c_int32) for n in ['local_id','bullet_lifetime','bomb_lifetime','proximity_bonus','proximity_delay','damage_divisor']]+[('explode',Explode),('sound',Sound),('random',Random),('damage',Damage),('user',C.c_void_p)]
ce=[];ne=[];random_value=0
@Explode
def explode(_,p):ce.append(('explode',));p.contents.word[0x18//4]=30
@Sound
def sound(_,x,y):ce.append(('sound',x,y))
@Random
def rand(_):ce.append(('random',random_value));return random_value
@Damage
def damage(_,s,amount,owner):ce.append(('damage',s.contents.word[0x44//4],amount,owner));s.contents.word[0x5c//4]-=amount
lib=C.CDLL(str(ROOT/('code/build/libprojectile_frame.so' if FRAME else 'code/build/libprojectile_update.so')));lib.sniper_projectile_update.argtypes=[C.POINTER(Projectile),C.POINTER(World),C.c_int32]
Release=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Projectile))
@Release
def release(_,p):ce.append(('release',(C.addressof(p.contents)-C.addressof(projectiles))//72))
if FRAME:lib.sniper_projectile_world_frame.argtypes=[C.POINTER(World),C.POINTER(C.c_int32),Release,C.c_void_p]
ships=(Ship*3)();projectiles=(Projectile*3)();sptrs=(C.POINTER(Ship)*3)(*(C.pointer(s) for s in ships));pptrs=(C.POINTER(Projectile)*3)(*(C.pointer(p) for p in projectiles))
grid=bytearray(1024*1024)
for y in range(20,31):
    for x in range(20,31):
        if x in [20,30] or y in [20,30]:grid[y*1024+x]=[1,152,255][(x+y)%3]
mapbuf=(C.c_uint8*len(grid)).from_buffer_copy(grid);coverage=(C.c_uint8*8192)()
w=World(C.addressof(mapbuf),C.addressof(coverage),32,sptrs,3,pptrs,3,1,500,6000,3,20,220000,explode,sound,rand,damage,None)
for a,v in [(0x429f48,0x600000),(0x429f50,0x700000),(0x42de7c,32),(0x429328,3),(0x4289f0,3),(0x429318,1),
 (0x4281e1,500),(0x4281e5,6000),(0x428269,3),(0x4281e9,20),(0x4281d1,220000)]:put(a,v)
u.mem_write(0x600000,bytes(grid))
def hook(uc,a,size,data):
    if FRAME and a in [0x416620,0x416280]:
        sp=uc.reg_read(UC_X86_REG_ESP)
        if a==0x416620:ne.append(('release',(get(sp+4)-0x500000)//72))
        else:
            dst,src,length=get(sp+4),get(sp+8),get(sp+12)
            if length:uc.mem_write(dst,bytes(uc.mem_read(src,length)))
        uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4);return
    if a not in [0x413510,0x407a80,0x4165f0,0x414b10]:return
    sp=uc.reg_read(UC_X86_REG_ESP);p=uc.reg_read(UC_X86_REG_ECX);clean=0;value=0
    if a==0x413510:ne.append(('explode',));put(p+0x18,30)
    elif a==0x407a80:ne.append(('sound',get(sp+8),get(sp+12)))
    elif a==0x4165f0:value=random_value;ne.append(('random',value))
    else:
        amount,owner=get(sp+4),get(sp+8);ne.append(('damage',get(p+0x44),amount,owner));put(p+0x5c,get(p+0x5c)-amount);clean=8
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(413010);seen=set()
for case in range(1800):
    if FRAME:
        w.projectile_count=3;put(0x4289f0,3)
        for i in range(3):pptrs[i]=C.pointer(projectiles[i])
    for i in range(3):
        p=projectiles[i]
        for j in range(18):p.word[j]=rng.randint(-10,100)
        for off,val in [(0,7),(4,rng.choice([400000,320000,480000,-1,16383999])),(8,rng.choice([400000,320000,480000,-1,16383999])),
            (0xc,rng.choice([0,-5000,5000])),(0x10,rng.choice([0,-5000,5000])),(0x14,rng.choice([0,499,5999])),(0x18,rng.choice([0,0,0,1,2,50])),
            (0x1c,rng.choice([1,2])),(0x20,7),(0x24,rng.choice([0,0,0,1,20])),(0x30,rng.choice([-1,0,1,50])),(0x34,case%6),(0x38,rng.randrange(4))]:p.word[off//4]=val
        u.mem_write(0x500000+i*72,bytes(p));put(0x429748+i*4,0x500000+i*72)
    for i in range(3):
        s=ships[i];C.memset(C.byref(s),0,536)
        for off,val in [(0x44,i+1),(0x48,projectiles[0].word[1]+rng.randint(-20000,20000)),(0x4c,projectiles[0].word[2]+rng.randint(-20000,20000)),
            (0x68,rng.choice([7,8])),(0xa4,rng.choice([0,0,1,2])),(0x5c,1000000)]:s.word[off//4]=val
        u.mem_write(0x510000+i*536,bytes(s));put(0x4283e8+i*4,0x510000+i*536)
    C.memset(coverage,0xff if case%3 else 0,8192);u.mem_write(0x700000,bytes(coverage))
    dt=rng.choice([0,1,2,5,10]);random_value=rng.randrange(32768);put(0x800000,0x810000);put(0x800004,dt)
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000);ce.clear();ne.clear()
    if FRAME:
        put(0x4218c8,dt);u.emu_start(0x40229b,0x4022f2,count=1000000)
        delta=C.c_int32(dt);lib.sniper_projectile_world_frame(C.byref(w),C.byref(delta),release,None)
        assert w.projectile_count==get(0x4289f0),(case,'count')
        for i in range(3):assert (C.addressof(pptrs[i].contents)-C.addressof(projectiles))//72==(get(0x429748+4*i)-0x500000)//72,(case,'list',i)
    else:
        u.emu_start(0x413010,0x810000,count=1000000);expected=u.reg_read(UC_X86_REG_EAX)
        result=lib.sniper_projectile_update(C.byref(projectiles[0]),C.byref(w),dt)
        assert result==expected,(case,'return',result,expected)
    assert bytes(projectiles)==bytes(u.mem_read(0x500000,C.sizeof(projectiles))),(case,'projectiles')
    assert bytes(ships)==bytes(u.mem_read(0x510000,C.sizeof(ships))),(case,'ships')
    assert ce==ne,(case,ce,ne)
    seen.update(e[0] for e in ce)
assert seen==({'explode','sound','random','damage','release'} if FRAME else {'explode','sound','random','damage'}),seen
print('1800 three-projectile frames match all projectile/ship bytes,list/count/stale tails and ordered effects' if FRAME else '1800 full projectile updates match all projectile/ship bytes,removal result and ordered external effects')
