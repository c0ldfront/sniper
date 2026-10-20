"""Offline constructor composition with file bytes supplied at stream boundary."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,128)).split(b'\0')[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Ptr=C.POINTER(Ship)
class Defaults(C.Structure):_fields_=[('v',C.c_int32*10)]
class Identity(C.Structure):_fields_=[('id',C.c_int32),('name',C.c_char_p),('squad',C.c_char_p)]+[(n,C.c_int32) for n in ('type','a','b','c','team','attached','d')]
class Hud(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
Alloc=C.CFUNCTYPE(C.c_void_p,C.c_void_p)
Release=C.CFUNCTYPE(None,C.c_void_p,Ptr)
class Registry(C.Structure):
    _fields_=[('players',C.POINTER(Ptr)),('ordered',C.POINTER(Ptr)),('local',Ptr)]+[(n,C.c_int32) for n in ('count','selected','dirty')]+[('hud',C.POINTER(Hud)),('defaults',C.POINTER(Defaults)),('ms',C.c_uint32),('allocate',Alloc),('release',Release),('user',C.c_void_p),('clock_ms',C.c_void_p),('clock_user',C.c_void_p)]
ce=[];ne=[]
@Alloc
def allocate(_):ce.append(('allocate',536));return C.addressof(ships[32])
@Release
def release(_,p):ce.append(('release',(C.addressof(p.contents)-C.addressof(ships))//536))
lib=C.CDLL(str(ROOT/'code/build/libplayer_registry.so'))
lib.sniper_player_add.argtypes=[C.POINTER(Registry),C.POINTER(Identity)];lib.sniper_player_add.restype=Ptr
lib.sniper_player_remove.argtypes=[C.POINTER(Registry),C.c_int32]
put(0x4353f4,0x530000);put(0x423f98,0)
class Size(C.Structure):_fields_=[('width',C.c_int32),('height',C.c_int32)]
init=C.CDLL(str(ROOT/'code/build/liboffline_init.so'))
init.sniper_offline_init.argtypes=[C.POINTER(Registry),C.c_void_p,C.c_void_p,C.POINTER(Size),C.c_void_p,C.c_size_t,C.c_uint32,C.POINTER(C.c_uint32),C.c_char_p,C.c_int32,C.c_int32,C.c_int32]
init.sniper_offline_init.restype=Ptr
sizes=(Size*256)()
for tile,side in [(216,16),(217,32),(218,64),(219,96),(250,16)]:
    sizes[tile]=Size(side,side);p=0x550000+tile*256
    put(0x42da18+tile*4,p);put(p+0x84,side);put(p+0x88,side)
put(0x429f48,0x600000)
reads=0
def hook(uc,a,size,user):
    global reads
    if a not in (0x416b20,0x530000,0x40a290,0x40a530,0x40a540):return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;value=0
    if a==0x416b20:
        amount=get(sp+4);assert amount in (272,536)
        value=0x540000 if amount==272 else 0x500000+32*536
    elif a==0x530000:
        value=generation_ms if reads==0 else milliseconds;reads+=1
    elif a==0x40a290:
        assert string(get(sp+4))==b'local.lvl'
        value=uc.reg_read(UC_X86_REG_ECX);clean=4
    elif a==0x40a530:value=len(data)//4
    else:
        index=get(sp+4);u.mem_write(get(sp+8),data[index*4:index*4+4]);clean=8
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x410ff0)
for case in range(102):
    if case<100:
        words=[]
        for i in range(case%30):
            x=rng.randrange(1000);y=rng.randrange(1000);tile=rng.choice([0,1,2,152,216,217,218,219])
            words.append(x|(y<<12)|(tile<<24))
        data=b''.join(struct.pack('<I',w) for w in words)+b'abc'[:case%4]
    else:data=(ROOT/'code/build/assets/files'/['LOCAL.LVL','45538382.LVL'][case-100]).read_bytes()
    ships=(Ship*33)();C.memset(ships,0x55,C.sizeof(ships))
    players=(Ptr*1024)();ordered=(Ptr*1024)();hud=Hud();defaults=Defaults()
    milliseconds=rng.getrandbits(32);generation_ms=rng.getrandbits(32)
    w=Registry(players,ordered,None,0,0,-7,C.pointer(hud),C.pointer(defaults),milliseconds,allocate,release,None)
    config=(C.c_int32*72)();grid=(C.c_uint8*1048576)();tick=C.c_uint32()
    name=b'Pilot';type_=case%5;team=case%7;field70=case%3
    u.mem_write(0x500000,bytes(ships));u.mem_write(0x600000,bytes(grid));u.mem_write(0x700000,name+b'\0')
    u.mem_write(0x429338,bytes(4096));put(0x429f48,0x600000);put(0x429328,0);put(0x4218e0,0);put(0x4289f4,-7);put(0x429f58,0)
    for i,v in enumerate([0x810000,0x700000,type_,team,field70]):put(0x800000+4*i,v)
    reads=0;u.reg_write(UC_X86_REG_ECX,0x520000);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x410ff0,0x810000,count=20000000)
    raw=C.create_string_buffer(data)
    result=init.sniper_offline_init(C.byref(w),config,grid,sizes,raw,len(data),generation_ms,C.byref(tick),name,type_,team,field70)
    assert bool(result) and C.addressof(result.contents)==C.addressof(ships[32])
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),case
    assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),(case,'map',data.hex(),[(i,a,b) for i,(a,b) in enumerate(zip(bytes(grid),bytes(u.mem_read(0x600000,len(grid))))) if a!=b][:10])
    assert bytes(config)==bytes(u.mem_read(0x4281c9,288)),(case,'config')
    assert tick.value==(get(0x520008)&0xffffffff)
    assert (w.count,w.selected,w.dirty)==(get(0x429328),get(0x429f58),get(0x4289f4))
    assert not w.local and C.addressof(players[0].contents)==C.addressof(ships[32])
    assert get(0x429338)==get(0x4283e8)==0x500000+32*536
print('102 offline initializations match original constructor map,config,ship,registry and generation timer;includes both shipped levels;file access supplied')
