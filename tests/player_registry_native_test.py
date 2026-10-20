"""Actual404340 constructor/notice/sort and404600 removal; allocator/clock seams."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
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
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
@Clock
def live_clock(_):ce.append(("clock",));return (milliseconds+1009*sum(e[0]=="clock" for e in ce))&0xffffffff
@Alloc
def allocate(_):ce.append(('allocate',536));return C.addressof(ships[32])
@Release
def release(_,p):ce.append(('release',(C.addressof(p.contents)-C.addressof(ships))//536))
lib=C.CDLL(str(ROOT/'code/build/libplayer_registry.so'))
lib.sniper_player_add.argtypes=[C.POINTER(Registry),C.POINTER(Identity)];lib.sniper_player_add.restype=Ptr
lib.sniper_player_remove.argtypes=[C.POINTER(Registry),C.c_int32]
put(0x4353f4,0x530000);put(0x423f98,0)
def hook(uc,a,size,user):
    if a not in (0x416b20,0x416620,0x416bc0,0x530000):return
    sp=uc.reg_read(UC_X86_REG_ESP);value=0
    if a==0x416b20:ne.append(('allocate',get(sp+4)));value=0x500000+32*536
    elif a==0x416620:ne.append(('release',(get(sp+4)-0x500000)//536))
    elif a==0x530000:ne.append(("clock",));value=(milliseconds+1009*sum(e[0]=="clock" for e in ne))&0xffffffff
    else:
        assert string(get(sp+8))==b'%s entering game'
        text=string(get(sp+12))+b' entering game';u.mem_write(get(sp+4),text+b'\0');value=len(text)
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x404340)
for case in range(500):
    ships=(Ship*33)();players=(Ptr*1024)();ordered=(Ptr*1024)();count=case%32
    for i,s in enumerate(ships):
        C.memset(C.byref(s),0x55,536)
        name=rng.choice([b'ALPHA',b'alpha',b'Zulu',b'Beta']);C.memmove(C.byref(s),name+b'\0',len(name)+1)
        s.word[0x44//4]=i;s.word[0x68//4]=i%3
        players[i]=C.pointer(s);put(0x429338+4*i,0x500000+i*536)
    order=list(range(count));rng.shuffle(order)
    for i,j in enumerate(order):ordered[i]=C.pointer(ships[j]);put(0x4283e8+4*i,0x500000+j*536)
    defaults=Defaults((C.c_int32*10)(100,1000000,160,30,2000,100,30,250,100000,10))
    for a,v in zip([0x428251,0x42824d,0x428241,0x428245,0x428249,0x428255,0x428259,0x42825d,0x428261,0x428265],defaults.v):put(a,v)
    hud=Hud();C.memset(C.byref(hud),0x41,C.sizeof(hud))
    for i in range(4):hud.ticks[i]=rng.choice([-1,0,10]);hud.color[i]=3
    milliseconds=rng.getrandbits(32)
    w=Registry(players,ordered,C.pointer(ships[0]) if count and case%3 else None,count,rng.randrange(count+3),-7,C.pointer(hud),C.pointer(defaults),0,allocate,release,None,C.cast(live_clock,C.c_void_p),None)
    ident=Identity(32,b'NewPilot',b'Team',case%5,11,22,33,case%3,-1,99)
    u.mem_write(0x500000,bytes(ships));u.mem_write(0x700000,ident.name+b'\0');u.mem_write(0x701000,ident.squad+b'\0')
    for data,a in [(hud.ticks,0x424330),(hud.text,0x4287e8),(hud.color,0x428a50)]:u.mem_write(a,bytes(data))
    for a,v in [(0x4218e0,0x500000 if w.local else 0),(0x429328,count),(0x429f58,w.selected),(0x4289f4,w.dirty)]:put(a,v)
    for i,v in enumerate([0x810000,32,0x700000,0x701000,ident.type,11,22,33,ident.team,-1,99]):put(0x800000+4*i,v)
    ce.clear();ne.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x404340,0x810000,count=2000000)
    result=lib.sniper_player_add(C.byref(w),C.byref(ident));assert C.addressof(result.contents)==C.addressof(ships[32])
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),case
    for data,a in [(hud.ticks,0x424330),(hud.text,0x4287e8),(hud.color,0x428a50)]:assert bytes(data)==bytes(u.mem_read(a,C.sizeof(data))),case
    assert ce==ne,(ce,ne)
    for remove_id in [case%33,32,100]:
        assert (w.count,w.selected,w.dirty)==tuple(get(a) for a in (0x429328,0x429f58,0x4289f4))
        assert [(C.addressof(ordered[i].contents)-C.addressof(ships))//536 for i in range(w.count)]==[(get(0x4283e8+4*i)-0x500000)//536 for i in range(w.count)]
        put(0x800000,0x810000);put(0x800004,remove_id);ce.clear();ne.clear();u.reg_write(UC_X86_REG_ESP,0x800000)
        u.emu_start(0x404600,0x810000,count=100000)
        lib.sniper_player_remove(C.byref(w),remove_id)
        assert not players[remove_id] and get(0x429338+remove_id*4)==0
        assert ce==ne,(ce,ne)
    assert (w.count,w.selected,w.dirty)==tuple(get(a) for a in (0x429328,0x429f58,0x4289f4))
print('500 registry additions and1500 removals match native ship bytes,notices,list order,selection,dirty and allocator traces;actual constructor/sort')
