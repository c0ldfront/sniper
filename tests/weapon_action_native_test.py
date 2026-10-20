"""406431..406922 actual action rules, notice storage; weapon creation captured."""
import sys
ORDINARY="--ordinary" in sys.argv
INPUT="--input" in sys.argv or ORDINARY
DISPATCH="--dispatch" in sys.argv or INPUT
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
class Hud(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
Weapon=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int32))
class World(C.Structure):_fields_=[('local',C.POINTER(Ship)),('config',C.c_void_p),('map',C.c_void_p),('projectiles',C.POINTER(C.POINTER(Projectile)))]+[(n,C.c_int32) for n in ['count','id','cooldown','safe','dirty']]+[('notices',C.POINTER(Hud)),('weapon',Weapon),('user',C.c_void_p)]
key_c=[];key_n=[];keys={}
Key=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_int32)
class Input(C.Structure):_fields_=[(n,C.c_int32) for n in ['joy','buttons','pov','mouse','x','y','height']]+[('key',Key),('user',C.c_void_p)]
@Key
def key_state(_,vk):key_c.append(vk);return keys.get(vk,0)
ce=[];ne=[];mine=0;backend_c=[];backend_n=[];native_fields=[]
Send=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int32))
class Position(C.Structure):_fields_=[('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ['interval','ticks','last_ticks','last_vx','last_vy','last_heading','dirty']]+[('send',Send),('user',C.c_void_p)]
@Send
def send(_,args):backend_c.append(tuple(args[i] for i in range(13)))
@Weapon
def weapon(_,args):ce.append(tuple(args[i] for i in range(14)))
lib=C.CDLL(str(ROOT/'code/build/libweapon_action.so'));lib.sniper_weapon_action_fields.argtypes=[C.POINTER(World),C.c_int,C.c_int32,C.c_int,C.POINTER(C.c_int32)]
control=C.CDLL(str(ROOT/'code/build/libweapon_control.so')) if DISPATCH else None
if DISPATCH:control.sniper_weapon_action_dispatch.argtypes=[C.POINTER(World),C.POINTER(Position),C.c_int,C.c_int32,C.c_int,C.POINTER(C.c_int32)]
if INPUT:control.sniper_weapon_control.argtypes=[C.POINTER(World),C.POINTER(Position),C.POINTER(Input),C.c_int32,C.POINTER(C.c_int32)]
if ORDINARY:
    Damage=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,C.c_int32)
    Poll=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
    Stage=C.CFUNCTYPE(None,C.c_void_p,C.c_int)
    class Lifecycle(C.Structure):
        _fields_=[('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ('key','repeat','mx','my','joystick_enabled','local_id')]+[('character',C.c_uint8),('joystick',C.c_uint32*13),('damage',Damage),('poll',Poll),('stage',Stage),('user',C.c_void_p)]
    class Effects(C.Structure):
        _fields_=[(n,C.c_int32) for n in ['thrust','boosted','exhaust','sound_count']]+[('slot',C.c_int32*3),('play',C.c_int32*3)]
    class Ordinary(C.Structure):
        _fields_=[('lifecycle',C.POINTER(Lifecycle)),('weapons',C.POINTER(World)),('position',C.POINTER(Position)),('input',Input)]+[(n,C.c_int32) for n in ['ticks','engine','exempt','exhaust']]+[('fields',C.c_int32*6),('effects',Effects),('dispatched',C.c_int),('audio_effects',C.c_void_p),('audio_user',C.c_void_p)]
    lifecycle_c=[];lifecycle_n=[];sound_n=[];joy_values=[];poll_status=0
    @Damage
    def damage(ctx,ship,amount,owner):
        assert ctx==1234
        lifecycle_c.append(('damage',amount,owner));ship.contents.word[0x5c//4]=-1
    @Poll
    def poll(ctx,idx,info):
        assert ctx==1234
        lifecycle_c.append(('poll',idx,tuple(info[i] for i in range(13))))
        for i,v in enumerate(joy_values):info[i]=v
        return poll_status
    @Stage
    def unused_stage(ctx,stage):raise AssertionError('original stage must be replaced')
    ordinary_lib=C.CDLL(str(ROOT/'code/build/libordinary_control.so'))
    ordinary_lib.sniper_ordinary_control.argtypes=[C.POINTER(Ordinary)]
s=Ship();config=(C.c_int32*72)();lib.sniper_local_config(config);grid=(C.c_uint8*(1024*1024))();objects=(Projectile*5)();ptrs=(C.POINTER(Projectile)*5)(*(C.pointer(p) for p in objects));hud=Hud();w=World(C.pointer(s),C.addressof(config),C.addressof(grid),ptrs,5,0,0,0,0,C.pointer(hud),weapon,None)
put(0x4218e0,0x500000);put(0x429f48,0x600000);put(0x4354b8,0x530000);put(0x429318,0);put(0x4292e8,0);put(0x4289f0,5)
position=Position(C.pointer(s),10,100,0,0,0,0,0,send,None)
for a,v in [(0x424348,0x520000),(0x520000,0x520100),(0x52010c,0x530100)]:put(a,v)
def hook(uc,a,size,data):
    if ORDINARY:
        if a in (0x405f97,0x406a9f):uc.emu_stop();return
        if a in (0x414b10,0x530200,0x40aae0,0x40aab0):
            sp=uc.reg_read(UC_X86_REG_ESP);clean=0
            if a==0x414b10:
                lifecycle_n.append(('damage',get(sp+4),get(sp+8)));put(0x500000+0x5c,-1);clean=8
            elif a==0x530200:
                idx,ptr=get(sp+4),get(sp+8)
                lifecycle_n.append(('poll',idx,tuple(get(ptr+4*i)&0xffffffff for i in range(13))))
                u.mem_write(ptr,struct.pack('<13I',*joy_values));uc.reg_write(UC_X86_REG_EAX,poll_status);clean=8
            else:
                from unicorn.x86_const import UC_X86_REG_ECX
                slot=(uc.reg_read(UC_X86_REG_ECX)-0x540000)//256
                sound_n.append((slot,int(a==0x40aae0)))
                if a==0x40aae0:clean=16
            uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean);return
    if a==0x406922:
        sp=uc.reg_read(UC_X86_REG_ESP)
        native_fields[:]=[uc.reg_read(UC_X86_REG_ESI)]+[get(sp+off) for off in [0x20,0x1c,0x14,0x10,0x18]]
        return
    if a==0x530100:
        sp=uc.reg_read(UC_X86_REG_ESP);backend_n.append(tuple(get(sp+4+i*4) for i in range(13)))
        uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+56);return
    if a not in [0x530000,0x409d70]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x530000:
        vk=get(sp+4)
        if INPUT:key_n.append(vk)
        uc.reg_write(UC_X86_REG_EAX,keys.get(vk,0) if INPUT else 0x8000 if mine else 0);clean=4
    else:ne.append(tuple(get(sp+4+i*4) for i in range(14)))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x406431);types=set();mine_notices=0
for case in range(2400):
    C.memset(C.byref(s),0,536)
    for off,val in [(0x48,rng.choice([400000,-1,-16000,16384000])),(0x4c,400000),(0x50,200),(0x54,-100),(0x58,rng.choice([12345,-1999,39999])),(0x5c,rng.choice([-1,0,20000,400000,2000000])),(0x68,1)]:s.word[off//4]=val
    for off in [0xd0,0xd4,0xd8,0xdc]:s.word[off//4]=rng.randrange(4)
    for off in [0xe8,0xec,0xf0,0xf4,0xf8,0xfc,0x100,0x88,0x8c,0x90,0x94,0x98]:s.word[off//4]=rng.choice([-1,0,1])
    for i,p in enumerate(objects):
        p.word[8]=rng.choice([1,2]);p.word[3]=rng.choice([0,0,100]);p.word[4]=0
        u.mem_write(0x510000+i*72,bytes(p));put(0x429748+4*i,0x510000+i*72)
    config[(0x4282b5-0x4281c9)//4]=rng.choice([0,1,3,10]);u.mem_write(0x4281c9,bytes(config))
    grid[25*1024+25]=rng.choice([0,0,152]);grid[25*1024]=grid[25*1024+25];u.mem_write(0x600000,bytes(grid))
    C.memset(C.byref(hud),0x41,C.sizeof(hud))
    for i in range(4):hud.ticks[i]=rng.choice([-1,0,10]);hud.color[i]=rng.randrange(4)
    u.mem_write(0x424330,bytes(hud.ticks));u.mem_write(0x4287e8,bytes(hud.text));u.mem_write(0x428a50,bytes(hud.color));u.mem_write(0x500000,bytes(s))
    w.cooldown=rng.choice([-1,0,0,1]);w.safe=rng.choice([-1,0,10]);w.dirty=0
    for a,v in [(0x429300,w.cooldown),(0x4283d8,w.safe),(0x4292fc,w.dirty)]:put(a,v)
    position.dirty=w.dirty
    position.ticks=case%110;position.last_ticks=case%17;position.last_vx=200;position.last_vy=-100;position.last_heading=s.word[0x58//4]
    for a,v in [(0x4218cc,position.ticks),(0x4281c0,position.last_ticks),(0x428a68,position.last_vx),(0x428a00,position.last_vy),(0x428a70,position.last_heading)]:put(a,v)
    position.interval=config[(0x428205-0x4281c9)//4];backend_c.clear();backend_n.clear();native_fields.clear()
    active=case%5!=0;action=case%8;mine=case%3==0
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EBP,int(active));u.reg_write(UC_X86_REG_EDI,action);u.reg_write(UC_X86_REG_EBX,0);ce.clear();ne.clear()
    fields=(C.c_int32*6)(0,*[rng.randrange(-9,10) for _ in range(5)])
    inp=Input(case%2,case%16,[-1,0,9000,18000,27000][case%5],int(active),[0,59,60][case%3],240+action*8+1,480,key_state,None)
    ticks=case%5;keys={16:0x8000 if mine else 0,17:0x8000 if case%5==0 else 0,9:0x8000 if case%7==0 else 0}
    key_c.clear();key_n.clear()
    if INPUT:
        fields[3]=inp.mouse
        for a,v in [(0x4218c8,ticks),(0x422278,inp.joy),(0x4292e8,inp.buttons),(0x4292f0,inp.pov),(0x42229c,inp.x),(0x4222a0,inp.y),(0x42434c,inp.height)]:put(a,v)
    for off,v in zip([0x20,0x1c,0x14,0x10,0x18],list(fields)[1:]):put(0x800000+off,v)
    if ORDINARY:
        for off,val in [(0x40,case%4),(0xcc,66),(0xe0,40),(0xe4,3000),(0x6c,-1 if case%3 else 0),(0x104,case%4==0),(0x214,case%12)]:s.word[off//4]=val
        u.mem_write(0x500000,bytes(s))
        for vk in (38,40,37,39):keys[vk]=0x8000 if rng.randrange(2) else 0
        life=Lifecycle(C.pointer(s),123 if case%11==0 else 0,7,17,-5,inp.joy,0,65,(C.c_uint32*13)(*([0x12345678]*13)),damage,poll,unused_stage,1234)
        joy_values=[52,0xcc3,rng.randrange(65536),rng.randrange(65536),0,0,0,0,inp.buttons&0xffffffff,0,inp.pov&0xffffffff,0,0]
        poll_status=[0,165,167][case%3]
        ordinary=Ordinary(C.pointer(life),C.pointer(w),C.pointer(position),inp,ticks,case%2,1,case%11,fields,Effects(),0)
        for a,v in [(0x435580,0x530200),(0x422248,life.key),(0x42224c,life.repeat),(0x4222a4,life.mx),(0x4222a8,life.my),(0x42930c,1),(0x42833c,ordinary.exhaust),(0x42e144,ordinary.engine)]:put(a,v)
        for slot in range(3):put(0x4218fc+4*slot,0x540000+slot*256)
        u.mem_write(0x422244,bytes([life.character]));u.mem_write(0x4292c8,bytes(life.joystick))
        lifecycle_c.clear();lifecycle_n.clear();sound_n.clear()
        alive=s.word[0x5c//4]>=0
        u.reg_write(UC_X86_REG_FPCW,0x37f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    u.emu_start(0x405f75 if ORDINARY else 0x406332 if INPUT else 0x406431,0x406aa6 if ORDINARY else 0x406a84 if DISPATCH else 0x406922,count=100000)
    if DISPATCH:
        if ORDINARY:
            ran=ordinary_lib.sniper_ordinary_control(C.byref(ordinary));assert ran==alive
            sent=ordinary.dispatched;fields=ordinary.fields
            assert lifecycle_c==lifecycle_n,(case,'lifecycle',lifecycle_c,lifecycle_n)
            assert list(life.joystick)==list(struct.unpack('<13I',u.mem_read(0x4292c8,52)))
            assert [life.key,life.repeat,life.mx,life.my]==[get(a) for a in (0x422248,0x42224c,0x4222a4,0x4222a8)]
            assert life.character==u.mem_read(0x422244,1)[0] and life.user==1234
            assert ordinary.exhaust==get(0x42833c),(case,'exhaust timer')
            assert [(ordinary.effects.slot[i],ordinary.effects.play[i]) for i in range(ordinary.effects.sound_count)]==sound_n
            if not alive:native_fields[:]=list(fields)
        elif INPUT:sent=control.sniper_weapon_control(C.byref(w),C.byref(position),C.byref(inp),ticks,fields)
        else:sent=control.sniper_weapon_action_dispatch(C.byref(w),C.byref(position),active,action,mine,fields)
        assert key_c==key_n,(case,'key order')
        result=fields[0];assert sent==bool(backend_n) and backend_c==backend_n,(case,'backend',backend_c,backend_n)
        assert [position.last_ticks,position.last_vx,position.last_vy,position.last_heading,position.dirty]==[get(a) for a in [0x4281c0,0x428a68,0x428a00,0x428a70,0x4292fc]],(case,'position')
    else:
        result=lib.sniper_weapon_action_fields(C.byref(w),active,action,mine,fields)
        native_fields[:]=[u.reg_read(UC_X86_REG_ESI)]+[get(0x800000+off) for off in [0x20,0x1c,0x14,0x10,0x18]]
    assert list(fields)==native_fields,(case,'fields',list(fields),native_fields)
    assert bytes(s)==bytes(u.mem_read(0x500000,536)),(case,'ship')
    assert (w.cooldown,w.safe,w.dirty)==tuple(get(a) for a in [0x429300,0x4283d8,0x4292fc]),(case,'globals')
    for data,a in [(hud.ticks,0x424330),(hud.text,0x4287e8),(hud.color,0x428a50)]:assert bytes(data)==bytes(u.mem_read(a,C.sizeof(data))),(case,'hud')
    assert ce==ne,(case,ce,ne)
    types.add(result);mine_notices+=b'Team has too many mines' in bytes(hud.text)
assert types==set(range(6)),types
assert mine_notices>0
print(f'2400 original {"input+" if INPUT else ""}action{"+dispatch" if DISPATCH else ""} blocks match ship,HUD,cooldowns,safe/dirty state and weapon args;{mine_notices} mine-limit notices')

if ORDINARY:print("Includes full ordinary lifecycle, flight/exhaust, damage and joystick seams, cleanup and original key/sound order; speed-exempt")
