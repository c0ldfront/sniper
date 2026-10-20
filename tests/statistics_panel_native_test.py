"""Full408ac0 content/cache/scroll flow; CRT formatting and panel APIs controlled."""
import ctypes as C
import pathlib,random,re,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,4096);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Row(C.Structure):_fields_=[('text',C.c_char_p),('color',C.c_int32)]
class Panel(C.Structure):_fields_=[('valid',C.c_int32),('scroll',C.c_int32),('count',C.c_int32),('per',C.c_int32),('tick',C.c_uint32),('title',C.c_char*128),('text',(C.c_char*128)*256),('rows',Row*256)]
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
ms=0;ccalls=0;ncalls=0
@Clock
def clock(_):
    global ccalls
    v=(ms+ccalls*10)&0xffffffff;ccalls+=1;return v
lib=C.CDLL(str(root/'code/build/libstatistics_panel.so'));lib.sniper_statistics_update.argtypes=[C.POINTER(Panel),C.POINTER(C.POINTER(Ship)),C.c_int32,C.POINTER(Ship),C.c_int32,C.POINTER(C.c_int32),C.POINTER(C.c_int32),C.c_int,Clock,C.c_void_p]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,128)).split(b'\0')[0]
rows=[];creates=[];destroys=0;draws=[]
def hook(uc,a,size,user):
    global ncalls,destroys
    if a not in [0x416bc0,0x416b20,0x416620,0x412560,0x4125b0,0x412a20,0x412610,0x530000]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x416bc0:
        fmt=string(get(sp+8));types=re.findall(rb'%[-0-9.]*([scd])',fmt)
        args=[string(get(sp+12+i*4)) if t==b's' else get(sp+12+i*4) for i,t in enumerate(types)]
        value=fmt%tuple(args);u.mem_write(get(sp+4),value+b'\0');uc.reg_write(UC_X86_REG_EAX,len(value))
    elif a==0x416b20:uc.reg_write(UC_X86_REG_EAX,0x520000)
    elif a==0x412560:creates.append((string(get(sp+4)),get(sp+8)));uc.reg_write(UC_X86_REG_EAX,0x520000);clean=8
    elif a==0x4125b0:destroys+=1
    elif a==0x412a20:rows.append((string(get(sp+4)),get(sp+8)));clean=8
    elif a==0x530000:uc.reg_write(UC_X86_REG_EAX,(ms+ncalls*10)&0xffffffff);ncalls+=1
    elif a==0x412610:draws.append((get(sp+4),get(sp+8)));uc.emu_stop();return
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(800):
    ships=(Ship*12)();ptrs=(C.POINTER(Ship)*12)(*[C.pointer(s) for s in ships])
    for i,s in enumerate(ships):
        name=('Pilot'+str(i)+'x'*(case%20)).encode();squad=('Squad'+str(i)+'y'*(case%18)).encode()
        C.memmove(C.addressof(s),name+b'\0',len(name)+1);C.memmove(C.addressof(s)+32,squad+b'\0',len(squad)+1)
        s.word[0x40//4]=r.randrange(5);s.word[0x68//4]=r.randrange(3)
        for offset in [0x78,0x7c,0x80]:s.word[offset//4]=r.choice([-2147483648,-99,-1,0,1,100,999,2147483647])
        put(0x4283e8+4*i,0x500000+536*i)
    p=Panel();p.valid=case%2;p.scroll=r.randrange(20);ms=r.getrandbits(32);p.tick=(ms//10-r.choice([0,299,300,301,10000]))&0xffffffff
    history=r.randrange(1,17);selected=C.c_int32(r.randrange(40));dirty=C.c_int32(case%3==0);compact=(case//2)%2
    u.mem_write(0x500000,bytes(ships));put(0,0)
    for a,v in [(0x4218e0,0x500000),(0x429328,12),(0x429f58,selected.value),(0x42e538,history),(0x4282f0,p.scroll),(0x4289f4,dirty.value),(0x42dd74,p.tick),(0x42dd70,0x520000 if p.valid else 0),(0x4353f4,0x530000)]:put(a,v)
    rows.clear();creates.clear();draws.clear();destroys=ccalls=ncalls=0
    u.reg_write(UC_X86_REG_ESP,0x800000);put(0x800004,compact);u.emu_start(0x408ac0,0x408e4b,count=1000000)
    result=lib.sniper_statistics_update(C.byref(p),ptrs,12,C.byref(ships[0]),history,C.byref(selected),C.byref(dirty),compact,clock,None)
    assert result>=0,case
    assert (selected.value,dirty.value,p.scroll,p.tick)==(get(0x429f58),get(0x4289f4),get(0x4282f0),get(0x42dd74)&0xffffffff),case
    assert ccalls==ncalls and draws==[(0,8)],case
    if result:
        assert creates==[(p.title,p.per)],(case,creates,p.title,p.per)
        assert rows==[(p.rows[i].text,p.rows[i].color) for i in range(p.count)],(case,rows)
    else:assert not creates and not rows,case
for mode in [-1,0,1,2,3,4,5,2147483647]:
    dirty=C.c_int32(-9);put(0x4282ec,mode);put(0x4289f4,-9);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x4058d1,0x405d56,count=1000)
    assert lib.sniper_statistics_toggle(mode,C.byref(dirty))==get(0x4282ec) and dirty.value==1
print('PASS800 original statistics cases: full rows/title/color/rating, selection/scroll, cache expiry/dirty, clock order, placement and F2 cycle')
