"""Original online methods to return; intercept transport and local callbacks only."""
import ctypes as C,pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Packet(C.Structure):_fields_=[('data',C.c_uint8*512),('length',C.c_uint32),('reliable',C.c_uint32),('copy',C.c_uint32)]
lib=C.CDLL(str(root/'code/build/libnetwork_game_packets.so'))
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def ret(clean=0):
 sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,get(sp));u.reg_write(UC_X86_REG_ESP,sp+4+clean)
seen=[];effects=[];ms=0
def hook(uc,a,size,user):
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a==0x401610:
  assert uc.reg_read(UC_X86_REG_ECX)==0x510000
  p,n,rel,copy=[get(sp+4+i*4) for i in range(4)];seen.append((bytes(uc.mem_read(p,n)),rel,copy));ret(16)
 elif a==0x600000:uc.reg_write(UC_X86_REG_EAX,ms);ret()
 elif a in (0x401430,0x404910,0x404c50,0x404530,0x404fe0,0x404090):effects.append(a);ret()
 elif a==0x416190:uc.reg_write(UC_X86_REG_EAX,0x620000);ret()
 elif a==0x416170:ret()
u.hook_add(UC_HOOK_CODE,hook)
put(0x4353f4,0x600000);put(0x500010,0x510000);put(0x4218e0,0x520000)
def native(addr,args):
 seen.clear();effects.clear();u.mem_write(0x7ff000,b'\xa5'*0x2000);put(0x800000,0x700000)
 for i,a in enumerate(args):put(0x800004+4*i,a)
 u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000);u.emu_start(addr,0x700000,count=100000)
 return list(seen)
def value(p):return(bytes(p.data[:p.length]),p.reliable,p.copy)
r=random.Random(8703);count=0
for case in range(500):
 a=[r.randrange(-2**31,2**31) for _ in range(13)];ms=r.randrange(2**32);offset=r.randrange(2**32);put(0x5108ac,offset)
 p=Packet();lib.sniper_packet_position(C.byref(p),(C.c_int32*13)(*a),C.c_uint32(ms),C.c_uint32(offset));assert native(0x411860,a)==[value(p)],case;count+=1
 for name,addr,args in [('death',0x411970,a[:2]),('ship',0x411b10,a[:1]),('team',0x411c20,a[:1]),('attach',0x411cc0,a[:1])]:
  getattr(lib,'sniper_packet_'+name)(C.byref(p),*args);assert native(addr,args)==[value(p)],(case,name);count+=1
 reliable=r.randrange(3);player=a[3];put(0x500008,player);put(0x4282dd,reliable)
 lib.sniper_packet_pickup(C.byref(p),*a[:3],player,reliable);assert native(0x4119b0,a[:3])==[value(p)],case;count+=1
 text=bytes(r.randrange(32,127) for _ in range(r.choice([0,1,8,100,508])))+b'\0';u.mem_write(0x610000,text);put(0x4282d9,reliable)
 assert lib.sniper_packet_chat(C.byref(p),a[0],text,reliable)==1
 assert native(0x411a50,[a[0],0x610000])==[value(p)],case;count+=1
 pair=(Packet*2)();put(0x520068,a[0]);lib.sniper_packet_release_children(pair,a[0]);assert native(0x411c60,[])==list(map(value,pair)),case;count+=1
 slot=r.randrange(8);data=bytes(r.randrange(256) for _ in range(r.randrange(509)));u.mem_write(0x610000,data or b'x')
 cached=C.c_int(r.randrange(2));put(0x500014+4*slot,cached.value)
 assert lib.sniper_packet_cached(C.byref(p),a[0],slot,data,len(data),C.byref(cached))==1
 assert native(0x411b40,[a[0],slot,0x610000,len(data)])==[value(p)],case
 assert get(0x500014+4*slot)==cached.value;count+=1
 assert native(0x411b40,[a[0],slot,0,len(data)])==[];count+=1
assert lib.sniper_packet_chat(C.byref(p),0,b'x'*509,1)==0
cached=C.c_int(0);assert lib.sniper_packet_cached(C.byref(p),0,0,b'x'*509,509,C.byref(cached))==0
lib.sniper_packet_cached_into.restype=C.c_size_t
lib.sniper_packet_cached_into.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(C.c_uint32),C.c_int32,C.c_int32,C.c_void_p,C.c_size_t,C.POINTER(C.c_int)]
for n in [509,512,1024,4096,8192]:
 data=bytes(r.randrange(256) for _ in range(n));u.mem_write(0x610000,data);put(0x500014,0)
 output=(C.c_uint8*(n+4))();copy=C.c_uint32();cached=C.c_int(0)
 size=lib.sniper_packet_cached_into(output,n+4,C.byref(copy),-1,0,data,n,C.byref(cached))
 assert size==n+4 and native(0x411b40,[-1,0,0x610000,n])==[(bytes(output),1,copy.value)]
 assert cached.value==get(0x500014)==1;count+=1
print(f'PASS {count} full native online backend method cases: packet bytes, flags, cache state; local gameplay callbacks intercepted')
