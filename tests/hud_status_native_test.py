"""Differential draw/call trace of dynamic HUD407c00..407f9c."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
STACK,SHIP,TICK=0x1000000,0x1100000,0x1200000
for address in (STACK,SHIP,TICK):u.mem_map(address,65536)
def word(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def words(a,n):return struct.unpack('<'+'i'*n,u.mem_read(a,4*n))
def string(a):
    result=bytearray()
    while True:
        b=u.mem_read(a,1)[0]
        if not b:return result.decode('ascii')
        result.append(b);a+=1
word(0x4218e0,SHIP);word(0x4353f4,TICK);word(0x42931c,1234)
TEXT=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32,C.c_int)
RECT=C.CFUNCTYPE(None,C.c_void_p,*([C.c_int32]*4),C.c_uint32)
LINE=C.CFUNCTYPE(None,C.c_void_p,*([C.c_int32]*3),C.c_uint32)
CLOCK=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Hud(C.Structure):
    _fields_=[('local',C.POINTER(Ship))]+[(n,C.c_int32) for n in ('width','height','show_energy_bar','maximum_energy','blink','safety_ticks','player_count','connected')]+[(n,C.c_uint32) for n in ('energy_high','energy_medium','energy_low','border','capacity')]+[('text',TEXT),('rect',RECT),('vertical_line',LINE),('milliseconds',CLOCK),('last_receive_tick',CLOCK),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libhud_status.so'));lib.sniper_hud_status.argtypes=[C.POINTER(Hud)]
native=[];actual=[];now=last=0
@TEXT
def text(_,s,x,y,c):actual.append(('text',s.decode(),x,y,c))
@RECT
def rect(_,l,t,r,b,c):actual.append(('rect',l,t,r,b,c))
@LINE
def line(_,x,y,n,c):actual.append(('line',x,y,n,c))
@CLOCK
def milliseconds(_):actual.append(('now',));return now
@CLOCK
def receive(_):actual.append(('last',));return last
hooks={0x416bc0,0x415dd0,0x415d60,0x415e50,0x415c00,0x401df0,TICK}
def intercept(uc,addr,size,data):
    if addr not in hooks:return
    sp=uc.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0];result=0
    if addr==0x416bc0:
        dest,fmt=words(sp+4,2);fmt=string(fmt);count=fmt.count('%')
        value=fmt.replace('%02.2d','%02d')%words(sp+12,count)
        uc.mem_write(dest,value.encode()+b'\0');result=len(value)
    elif addr==0x415dd0:result=len(string(words(sp+4,1)[0]))*6
    elif addr==0x415d60:
        ptr,x,y,c=words(sp+4,4);native.append(('text',string(ptr),x,y,c))
    elif addr==0x415e50:
        target,ptr,c=words(sp+4,3);assert target==1234
        native.append(('rect',*words(ptr,4),c&0xffffffff))
    elif addr==0x415c00:
        target,x,y,n,c=words(sp+4,5);assert target==1234
        native.append(('line',x,y,n,c&0xffffffff))
    elif addr==TICK:native.append(('now',));result=now
    else:native.append(('last',));result=last
    uc.reg_write(UC_X86_REG_EAX,result&0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+4);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,intercept)
rng=random.Random(87)
energies=[-1,0,1,249999,250000,250001,499999,500000,500001,799999,800000,800001,1000000,2147483647]
for case in range(1400):
    ship=Ship();ship.word[0x5c//4]=energies[case%len(energies)]
    ship.word[0xc8//4]=rng.choice([250000,500000,1000000,2147483647])
    for off in (0x108,0x10c):ship.word[off//4]=rng.choice([-1,0,1,9,10,99,100,101,9999,2147483647])
    h=Hud(C.pointer(ship),rng.choice([319,640,1024]),rng.choice([200,481,768]),rng.randrange(2),1000000,rng.choice([-2147483648,-6,-1,0,1,5,6,2147483647]),rng.choice([-1,0,1,99,100,5999,6000,359999,360000,2147483647]),rng.randrange(4),rng.randrange(2),0x111,0x222,0x333,0x444,0x555,text,rect,line,milliseconds,receive,None)
    now=rng.choice([0,15000,15010,0xffffffff]);last=rng.choice([0,1,100,0xffffffff])
    u.mem_write(SHIP,bytes(ship))
    for addr,value in [(0x42818c,h.width),(0x42434c,h.height),(0x42e004,h.show_energy_bar),(0x428239,h.maximum_energy),(0x4219a4,h.blink),(0x4283d8,h.safety_ticks),(0x429328,h.player_count),(0x422238,h.connected),(0x424350,h.energy_high),(0x424328,h.energy_medium),(0x4281a8,h.energy_low),(0x429278,h.border),(0x429734,h.capacity)]:word(addr,value)
    native.clear();actual.clear();u.reg_write(UC_X86_REG_ESP,STACK+32768)
    u.emu_start(0x407c00,0x407f9c,count=100000)
    assert u.reg_read(UC_X86_REG_EIP)==0x407f9c
    lib.sniper_hud_status(C.byref(h))
    assert native==actual,(case,native,actual)
    assert h.blink==words(0x4219a4,1)[0],case
print('1400 dynamic HUD traces and blink states match original x86407c00..407f9c')
