"""Continuous generation/creation/expiry through actual original instructions."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,4096);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
class MovePickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['x','y','value']]
Allocate=C.CFUNCTYPE(C.c_void_p,C.c_void_p);Release=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Pickup));Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
pool=(Pickup*16)();cused=set();nused=set();cevents=[];nevents=[];times=[];ci=ni=0
@Allocate
def allocate(_):
    i=next(i for i in range(16) if i not in cused);cused.add(i);cevents.append(('allocate',i));return C.addressof(pool[i])
@Release
def release(_,p):
    i=(C.addressof(p.contents)-C.addressof(pool))//20;cused.remove(i);cevents.append(('release',i))
@Clock
def clock(_):
    global ci
    value=times[ci];ci+=1;cevents.append(('clock',value));return value
calls={0x404f10:0,0x412aa0:0,0x412b00:0,0x412ae0:0}
def hook(uc,a,size,data):
    global ni
    if a in calls:calls[a]+=1
    if a not in [0x820000,0x416b20,0x416620]:return
    sp=uc.reg_read(UC_X86_REG_ESP);value=0
    if a==0x820000:value=times[ni];ni+=1;nevents.append(('clock',value))
    elif a==0x416b20:
        assert get(sp+4)==20
        i=next(i for i in range(16) if i not in nused);nused.add(i);nevents.append(('allocate',i));value=0x530000+i*32
    else:
        i=(get(sp+4)-0x530000)//32;nused.remove(i);nevents.append(('release',i))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x820000);put(0x429f48,0x600000)
lib=C.CDLL(str(ROOT/'code/build/libpickup_sequence_test.so'))
lib.test_pickup_sequence_begin.argtypes=[C.c_void_p,C.c_uint32,C.c_uint32,Allocate,Release,Clock]
lib.test_pickup_sequence_step.argtypes=[C.c_int,C.c_int32,C.c_void_p,C.c_void_p,C.c_void_p]
rng=random.Random(87);results=(C.c_uint32*4)();output=(Pickup*16)();movement=(MovePickup*16)();count=0;full=0
for density in [0,0.8]:
    grid=bytearray(1024*1024)
    for y in range(380,644):
        for x in range(380,644):grid[y*1024+x]=1 if rng.random()<density else 0
    buf=(C.c_uint8*len(grid)).from_buffer_copy(grid);u.mem_write(0x600000,bytes(grid))
    seed=rng.getrandbits(32);put(0x422f78,seed);put(0x500008,0);put(0x428a08,0)
    cused.clear();nused.clear();lib.test_pickup_sequence_begin(buf,seed,0,allocate,release,clock)
    actions=[(1,0)]*20+[(0,20000)]+[(rng.randrange(2),rng.choice([0,50,500,6000,13000])) for _ in range(120)]
    ms=0
    for generate,dt in actions:
        ms=(ms+5010)&0xffffffff;times=[ms,(ms+3)&0xffffffff];ci=ni=0;cevents.clear();nevents.clear()
        put(0x800000,0x810000);put(0x4218c8,dt);u.reg_write(UC_X86_REG_ESP,0x800000)
        if generate:
            u.reg_write(UC_X86_REG_ECX,0x500000);u.emu_start(0x411450,0x810000,count=2000000)
        else:
            u.reg_write(UC_X86_REG_EAX,0);u.emu_start(0x4021d5,0x402237,count=2000000)
        lib.test_pickup_sequence_step(generate,dt,results,output,movement)
        assert list(results)==[get(0x422f78),get(0x500008),get(0x428a08),get(0x428a08)]
        assert cevents==nevents,(generate,dt,cevents,nevents)
        assert cused==nused
        for i in range(results[2]):
            assert bytes(output[i])==bytes(u.mem_read(get(0x429288+i*4),20))
            assert (movement[i].x,movement[i].y,movement[i].value)==(output[i].x,output[i].y,output[i].value)
        assert bytes(buf)==bytes(u.mem_read(0x600000,len(buf)))
        count+=1;full+=results[2]==16
assert full and all(calls.values())
print(f'{count} connected generation/expiry actions match objects,map,list,RNG,timestamp,allocation order and movement view; {full} full-list states; {calls}')
