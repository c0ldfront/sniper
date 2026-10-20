"""Full414b10, controlled RNG/audio/notice/backend callbacks."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Config(C.Structure):_fields_=[('word',C.c_int32*72)]
Random=C.CFUNCTYPE(C.c_int32,C.c_void_p)
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32)
Message=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32)
class World(C.Structure):_fields_=[('config',C.POINTER(Config)),('map',C.c_void_p),('local',C.c_int32),('random',Random),('sound',Sound),('death',Sound),('message',Message),('user',C.c_void_p)]
ce=[];ne=[];values=[];ci=0;ni=0
@Random
def rand(_):
    global ci
    v=values[ci];ci+=1;ce.append(('random',v));return v
@Sound
def sound(_,x,y):ce.append(('sound',x,y))
@Sound
def death(_,owner,bounty):ce.append(('death',owner,bounty))
@Message
def message(_,text,duration,color):ce.append(('message',text.decode(),duration,color))
lib=C.CDLL(str(ROOT/'code/build/libship_damage.so'));lib.sniper_ship_damage.argtypes=[C.POINTER(Ship),C.POINTER(World),C.c_int32,C.c_int32]
config=Config();lib.sniper_local_config(C.byref(config));grid=(C.c_uint8*(1024*1024))();w=World(C.pointer(config),C.addressof(grid),1,rand,sound,death,message,None)
put(0x429f48,0x600000);put(0x429318,1);put(0x424348,0x520000);put(0x520000,0x520100);put(0x520114,0x520200)
def hook(uc,a,size,data):
    global ni
    if a not in [0x4165f0,0x407a80,0x407b50,0x520200]:return
    sp=uc.reg_read(UC_X86_REG_ESP);value=0;clean=0
    if a==0x4165f0:value=values[ni];ni+=1;ne.append(('random',value))
    elif a==0x407a80:ne.append(('sound',get(sp+8),get(sp+12)))
    elif a==0x520200:ne.append(('death',get(sp+4),get(sp+8)));clean=8
    else:ne.append(('message',bytes(u.mem_read(get(sp+4),100)).split(b'\0')[0].decode(),get(sp+8),get(sp+12)))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x414b10);messages=set();events=set()
for case in range(2400):
    s=Ship()
    for j in range(134):s.word[j]=rng.randrange(100)
    # First768 exercise every downgrade with guaranteed probability success.
    full=case<768
    for off,val in [(0x44,rng.choice([1,2])),(0x48,rng.choice([400000,-1,-16000,16384000])),(0x4c,400000),(0x5c,1000000 if full else rng.choice([-1,0,100,10000,1000000])),(0x108,0 if full else rng.choice([-1,0,1])),(0x214,case%12),(0xa0,case%6)]:s.word[off//4]=val
    for off,base in [(0xc4,0x428251),(0xc8,0x42824d),(0xcc,0x428241),(0xe0,0x428245),(0xe4,0x428249)]:s.word[off//4]=config.word[(base-0x4281c9)//4]+rng.choice([0,1,10000])
    for off in [0xec,0xf0,0xf8,0xe8,0xfc,0x100,0xf4]:s.word[off//4]=rng.choice([0,1,-1])
    for off in [0xd0,0xd4,0xd8,0xdc]:s.word[off//4]=rng.choice([0,1,2,10])
    amount=1000 if full else rng.choice([-2,-1,0,1,100,10000,1000001]);owner=77
    grid[25*1024+25]=0 if full else rng.choice([0,152,255]);grid[25*1024]=grid[25*1024+25]
    u.mem_write(0x600000,bytes(grid));u.mem_write(0x4281c9,bytes(config));u.mem_write(0x500000,bytes(s))
    values=[0 if full else rng.randrange(32768),case%16];ci=ni=0;ce.clear();ne.clear()
    for a,v in [(0x800000,0x810000),(0x800004,amount),(0x800008,owner)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000)
    u.emu_start(0x414b10,0x810000,count=100000)
    lib.sniper_ship_damage(C.byref(s),C.byref(w),amount,owner)
    assert bytes(s)==bytes(u.mem_read(0x500000,536)),(case,[(i, s.word[i], get(0x500000+4*i)) for i in range(134) if s.word[i]!=get(0x500000+4*i)])
    assert ce==ne,(case,ce,ne)
    events.update(e[0] for e in ce);messages.update(e[1] for e in ce if e[0]=='message')
assert len(messages)==16,(len(messages),messages)
assert events=={'random','sound','death','message'},events
print('2400 full ship damage comparisons pass; all16 downgrade messages, full ship state and ordered effects match')
