"""Full415030 comparison, including strings, flip-to-positive and saved upgrades."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Random=C.CFUNCTYPE(C.c_int32,C.c_void_p)
Spawn=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int,C.c_int)
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
Message=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32)
class World(C.Structure):
    _fields_=[('config',C.POINTER(C.c_int32)),('positive_sound',C.c_int32),('negative_sound',C.c_int32),('warp_sound',C.c_int32),
        ('random',Random),('spawn',Spawn),('sound',Sound),('message',Message),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libprize.so'));lib.sniper_prize.argtypes=[C.POINTER(Ship),C.POINTER(World),C.c_int32]
config=(C.c_int32*72)();lib.sniper_local_config(config)
cevents=[];nevents=[];random_value=0
@Random
def rand(_):cevents.append(('random',random_value));return random_value
@Spawn
def spawn(_,s,reset,move):
    cevents.append(('spawn',reset,move));s.contents.word[0x48//4]=123456;s.contents.word[0xbc//4]=48
@Sound
def sound(_,handle):cevents.append(('sound',handle))
@Message
def message(_,text,duration,color):cevents.append(('message',text.decode(),duration,color))
w=World(config,101,102,103,rand,spawn,sound,message,None)
for a,v in [(0x42190c,101),(0x4218ec,102),(0x4218e8,103)]:put(a,v)
def hook(uc,a,size,data):
    if a not in [0x4165f0,0x4137c0,0x40aae0,0x407b50]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0;value=0
    if a==0x4165f0:value=random_value;nevents.append(('random',value))
    elif a==0x4137c0:
        assert uc.reg_read(UC_X86_REG_ECX)==0x500000
        nevents.append(('spawn',get(sp+4),get(sp+8)));put(0x500048,123456);put(0x5000bc,48);clean=8
    elif a==0x40aae0:
        assert [get(sp+i) for i in [4,8,12,16]]==[0]*4
        nevents.append(('sound',uc.reg_read(UC_X86_REG_ECX)));clean=16
    else:
        text=bytes(u.mem_read(get(sp+4),128)).split(b'\0',1)[0].decode()
        nevents.append(('message',text,get(sp+8),get(sp+12)))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp)&0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(415030);count=0
for code in list(range(-21,22))+[-22,-0x80000000]:
    for case in range(100):
        ship=Ship()
        for i in range(134):ship.word[i]=rng.choice([0,1,-1,2,3,rng.randint(-10000,4000000),0x7fffffff,-0x80000000])
        ship.word[0x40//4]=case%5
        for off,addr in [(0xc4,0x428251),(0xc8,0x42824d),(0xcc,0x428241),(0xe0,0x428245),(0xe4,0x428249)]:
            base=config[(addr-0x4281c9)//4]
            if case%4==0:ship.word[off//4]=base
            elif case%4==1:ship.word[off//4]=base+1
        random_value=rng.randrange(32768);u.mem_write(0x4281c9,bytes(config));u.mem_write(0x500000,bytes(ship))
        put(0x800000,0x810000);put(0x800004,code);u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000)
        cevents.clear();nevents.clear();u.emu_start(0x415030,0x810000,count=100000)
        assert u.reg_read(UC_X86_REG_EIP)==0x810000
        assert lib.sniper_prize(C.byref(ship),C.byref(w),code)==0
        expected=bytes(u.mem_read(0x500000,536))
        diffs=[(hex(i),get(0x500000+i),ship.word[i//4]) for i in range(0,536,4) if bytes(ship)[i:i+4]!=expected[i:i+4]]
        assert not diffs,(code,case,diffs)
        assert cevents==nevents,(code,case,cevents,nevents)
        count+=1
print(f'{count} full prize-effect cases match536 ship bytes and ordered random/spawn/sound/message calls')
