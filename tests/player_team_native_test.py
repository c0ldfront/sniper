"""Actual team handler,CRT sort and comparator; spawn/detach controlled."""
import ctypes as C
import pathlib,random,struct,sys
OFFLINE="--offline" in sys.argv
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Ptr=C.POINTER(Ship)
Detach=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
Spawn=C.CFUNCTYPE(None,C.c_void_p,Ptr,C.c_int,C.c_int)
class World(C.Structure):
    _fields_=[('players',C.POINTER(Ptr)),('ordered',C.POINTER(Ptr)),('local',Ptr)]+[(n,C.c_int32) for n in ('count','selected','dirty','reset')]+[('detach',Detach),('spawn',Spawn),('user',C.c_void_p)]
ce=[];ne=[];observed=set()
@Detach
def detach(_,id):ce.append(('detach',id));ships[0].word[0x6c//4]=-1
@Spawn
def spawn(_,s,reset,move):
    ce.append(('spawn',reset,move));s.contents.word[0x5c//4]=123;s.contents.word[0x58//4]=777
lib=C.CDLL(str(ROOT/'code/build/libplayer_team.so'));lib.sniper_player_team.argtypes=[C.POINTER(World),C.c_int32,C.c_int32]
for a,v in [(0x4218e0,0x500000),(0x424348,0x520000),(0x520000,0x520100),(0x52012c,0x530000),(0x423f98,0)]:put(a,v)
if OFFLINE:
    offline=C.CDLL(str(ROOT/'code/build/liboffline_team_test.so'))
    offline.test_offline_team.argtypes=[Ptr,C.POINTER(C.c_int32),C.c_int32,C.POINTER(C.c_int32)]
    config=(C.c_int32*72)();offline.sniper_local_config(config)
    put(0x52012c,0x411620);put(0x4353f4,0x530100);put(0x429318,0)
def hook(uc,a,size,user):
    if OFFLINE:
        if a in (0x4137c0,0x411620):observed.add(a)
        if a==0x530100:
            sp=uc.reg_read(UC_X86_REG_ESP);uc.reg_write(UC_X86_REG_EAX,milliseconds)
            uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
        return
    if a not in (0x530000,0x4137c0):return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x530000:
        ne.append(('detach',get(sp+4)));put(0x50006c,-1);clean=4
    else:
        assert uc.reg_read(UC_X86_REG_ECX)==0x500000
        ne.append(('spawn',get(sp+4),get(sp+8)));put(0x50005c,123);put(0x500058,777);clean=8
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x404530);spawns=detaches=0
for case in range(900):
    count=rng.choice([0,1,2,7,8,9,16,33,64]);ships=(Ship*65)();players=(Ptr*1024)()
    for i,s in enumerate(ships):
        name=rng.choice([b'alpha',b'ALPHA',b'z',b'Z',b'',bytes([rng.randrange(128,256)])]) if case%2 else ('Pilot%02d'%i).encode()
        C.memmove(C.byref(s),name+b'\0',len(name)+1)
        s.word[0x44//4]=i;s.word[0x68//4]=rng.randrange(4);s.word[0x5c//4]=1000000
        players[i]=C.pointer(s);put(0x429338+i*4,0x500000+536*i)
    id=0 if OFFLINE else rng.choice([0,1,64,100]);team=rng.randrange(-2,5)
    ships[0].word[0x6c//4]=rng.choice([-1,id,3]);put(0x429338+400,0)
    order=list(range(count));rng.shuffle(order);ordered=(Ptr*65)(*[C.pointer(ships[i]) for i in order])
    w=World(players,ordered,C.pointer(ships[0]),count,rng.randrange(count+3),0,case%2,detach,spawn,None)
    u.mem_write(0x500000,bytes(ships))
    for i,j in enumerate(order):put(0x4283e8+4*i,0x500000+536*j)
    for a,v in [(0x429328,count),(0x429f58,w.selected),(0x4289f4,0),(0x4282ad,w.reset)]:put(a,v)
    if OFFLINE:
        milliseconds=rng.getrandbits(32)
        values=(C.c_int32*7)(w.selected,w.dirty,w.reset,C.c_int32(milliseconds).value,12345,-7,team)
        order_values=(C.c_int32*65)(*order)
        config[(0x4282ad-0x4281c9)//4]=w.reset
        u.mem_write(0x4281c9,bytes(config));put(0x422f78,12345);put(0x4292fc,-7)
    put(0x800000,0x810000);put(0x800004,id);put(0x800008,team)
    ce.clear();ne.clear();u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x404530,0x810000,count=2000000)
    if OFFLINE:
        offline.test_offline_team(ships,order_values,count,values)
        for i in range(count):ordered[i]=C.pointer(ships[order_values[i]])
        w.selected=values[0];w.dirty=values[1]
        assert list(values)[4:6]==[get(0x422f78),get(0x4292fc)]
    else:lib.sniper_player_team(C.byref(w),id,team)
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),case
    actual=[(C.addressof(ordered[i].contents)-C.addressof(ships))//536 for i in range(count)]
    expected=[(get(0x4283e8+4*i)-0x500000)//536 for i in range(count)]
    assert actual==expected,(case,order,actual,expected)
    assert (w.selected,w.dirty)==(get(0x429f58),get(0x4289f4))
    assert ce==ne,(case,ce,ne)
    spawns+=sum(e[0]=='spawn' for e in ce);detaches+=sum(e[0]=='detach' for e in ce)
if not OFFLINE:print(f'900 team changes match original ship bytes,exact sorted order,selection,dirty flag and callbacks;{spawns} resets,{detaches} detaches;default C locale')

if OFFLINE:
    assert {0x4137c0,0x411620}<=observed,observed
    print("900 offline team changes match full ship state,exact sort,selection,RNG and dirty flags;actual attachment and spawn/reset observed;clock supplied")
