"""Original403e40 recovery traces,early exits,dirty flags and palette mapping."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
STACK,PRIMARY,BACK,TEAM,EQUIP,TILES,TILE_SURF,VT,RESTORE,RELEASE,PALETTE,STOP=[0x1000000+i*65536 for i in range(12)]
for a in (STACK,PRIMARY,BACK,TEAM,EQUIP,TILES,TILE_SURF,VT,RESTORE,RELEASE,PALETTE,STOP):u.mem_map(a,65536)
def word(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def words(a,n):return struct.unpack('<'+'I'*n,u.mem_read(a,4*n))
for a in (PRIMARY,BACK,TEAM,EQUIP):word(a,VT)
word(VT+0x6c,RESTORE);word(VT+8,RELEASE);word(VT+0x7c,PALETTE);word(TILES,TILE_SURF)
Handle=C.c_size_t
Restore=C.CFUNCTYPE(C.c_uint32,C.c_void_p,Handle)
Release=C.CFUNCTYPE(None,C.c_void_p,Handle)
Palette=C.CFUNCTYPE(C.c_uint32,C.c_void_p,Handle,Handle)
Color=C.CFUNCTYPE(C.c_uint32,C.c_void_p,Handle,C.c_uint32)
class State(C.Structure):_fields_=[(n,Handle) for n in ('primary','back','team','equipment','palette','tiles','tile_surface')]+[('dirty',C.c_int32*3),('colors',C.c_uint32*16),('restore',Restore),('release',Release),('set_palette',Palette),('restore_tiles',Restore),('color',Color),('user',C.c_void_p)]
actual=[];native=[];results={};color_results=[];ci=ni=0
@Restore
def restore(_,obj):actual.append(('restore',obj));return results[obj]
@Release
def release(_,obj):actual.append(('release',obj))
@Palette
def palette(_,obj,pal):actual.append(('palette',obj,pal));return results[PALETTE]
@Restore
def tiles(_,obj):actual.append(('tiles',obj));return results[TILES]
@Color
def color(_,obj,rgb):
    global ci
    actual.append(('color',obj,rgb));v=color_results[ci];ci+=1;return v
color_addrs=[0x42815c,0x424320,0x428338,0x429278,0x4289ec,0x42de78,0x428340,0x42932c,0x4281a8,0x429740,0x424350,0x424328,0x429734,0x4289e8,0x42834c,0x42431c]
def intercept(uc,a,size,user):
    global ni
    if a not in (RESTORE,RELEASE,PALETTE,0x4103e0,0x412300):return
    sp=uc.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0];pop=0;result=0
    if a==RESTORE:
        obj=words(sp+4,1)[0];native.append(('restore',obj));result=results[obj];pop=1
    elif a==RELEASE:native.append(('release',words(sp+4,1)[0]));pop=1
    elif a==PALETTE:native.append(('palette',*words(sp+4,2)));result=results[PALETTE];pop=2
    elif a==0x4103e0:native.append(('tiles',uc.reg_read(UC_X86_REG_ECX)));result=results[TILES]
    else:native.append(('color',*words(sp+4,2)));result=color_results[ni];ni+=1
    uc.reg_write(UC_X86_REG_ESP,sp+4+4*pop);uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,intercept)
lib=C.CDLL(str(ROOT/'code/build/libdisplay_recovery.so'));lib.sniper_display_recover.argtypes=[C.POINTER(State)]
rng=random.Random(87);success=0
for case in range(600):
    results={PRIMARY:rng.choice([0,0,0,1,0x887601c2]),BACK:rng.choice([0,0,0,1,0x80004005]),PALETTE:rng.getrandbits(32),TILES:rng.getrandbits(32)}
    color_results=[rng.getrandbits(32) for _ in range(16)]
    s=State(PRIMARY,BACK,TEAM if case%2 else 0,EQUIP if case%3 else 0,77,TILES,TILE_SURF,(C.c_int32*3)(-1,4,0),(C.c_uint32*16)(*([0x12345678]*16)),restore,release,palette,tiles,color,None)
    for a,v in [(0x42dd6c,PRIMARY),(0x42931c,BACK),(0x4281b0,s.team),(0x4283e0,s.equipment),(0x429330,77),(0x4281b4,TILES)]:word(a,v)
    for a,v in zip([0x428334,0x4289f4,0x428188],s.dirty):word(a,v)
    for a,v in zip(color_addrs,s.colors):word(a,v)
    actual.clear();native.clear();ni=ci=0;u.reg_write(UC_X86_REG_ESP,STACK+32768);word(STACK+32768,STOP)
    u.emu_start(0x403e40,STOP,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
    result=lib.sniper_display_recover(C.byref(s));success+=result
    assert result==u.reg_read(UC_X86_REG_EAX) and actual==native,(case,actual,native)
    assert list(s.colors)==[words(a,1)[0] for a in color_addrs]
    assert list(s.dirty)==[C.c_int32(words(a,1)[0]).value for a in [0x428334,0x4289f4,0x428188]]
    assert s.team==words(0x4281b0,1)[0] and s.equipment==words(0x4283e0,1)[0]
print(f'600 display recovery traces and states match original x86; {success} full color remaps,early restore failures preserved')
