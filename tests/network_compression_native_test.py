"""Native410920/410c60 codec, intercepting only CRT allocation/memmove/free.
Allocator zero-fills fresh memory; final unused bits otherwise contain heap data.
"""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/src/network_compression.c'),'-o',str(root/'code/build/libnetwork_compression.so')],check=True)
lib=C.CDLL(str(root/'code/build/libnetwork_compression.so'))
lib.sniper_network_compress.argtypes=[C.c_void_p,C.c_int32,C.POINTER(C.c_int32),C.c_int,C.c_uint32];lib.sniper_network_compress.restype=C.c_void_p
lib.sniper_network_decompress.argtypes=[C.c_void_p,C.c_int32,C.POINTER(C.c_int32)];lib.sniper_network_decompress.restype=C.c_void_p
crt=C.CDLL(None);crt.free.argtypes=[C.c_void_p]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_map(0x1000000,0x4000000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def put(a,n):u.mem_write(a,struct.pack('<I',n))
allocs={};heap=0x1000000

def hook(uc,a,sz,_):
 global heap
 if a not in (0x416190,0x416210,0x416b20,0x416170,0x416620,0x416280):return
 sp=uc.reg_read(UC_X86_REG_ESP);ret=get(sp);p=get(sp+4);result=0
 if a==0x416280:
  n=get(sp+12);u.mem_write(p,bytes(u.mem_read(get(sp+8),n)));result=p
 elif a not in (0x416170,0x416620):
  n=get(sp+8) if a==0x416210 else p
  result=heap;heap+=(max(n,1)+15)&~15
  assert heap<0x5000000
  u.mem_write(result,bytes(max(n,1)))
  if a==0x416210 and p:
   old=allocs[p];u.mem_write(result,bytes(u.mem_read(p,min(old,n))))
  allocs[result]=n
 uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,hook)
def native(data,decode=False,bypass=0,passes=1):
 global heap
 heap=0x1000000;allocs.clear();u.mem_write(0x600000,data or b'\0');allocs[0x600000]=len(data)
 args=[0x600000,len(data),0x610000]+([] if decode else [bypass,passes])
 u.mem_write(0x800000,struct.pack('<'+'I'*(len(args)+1),0x700000,*args));u.reg_write(UC_X86_REG_ESP,0x800000)
 u.emu_start(0x410c60 if decode else 0x410920,0x700000,count=30000000)
 assert u.reg_read(UC_X86_REG_EIP)==0x700000
 return bytes(u.mem_read(u.reg_read(UC_X86_REG_EAX),get(0x610000)))
def portable(data,decode=False,bypass=0,passes=1):
 n=C.c_int32();p=lib.sniper_network_decompress(data,len(data),C.byref(n)) if decode else lib.sniper_network_compress(data,len(data),C.byref(n),bypass,passes)
 assert p
 result=C.string_at(p,n.value);crt.free(p);return result
r=random.Random(87);cases=[]
for i in range(100):
 n=r.choice([1,2,10,771,772,773,1024,2048,8193]);alphabet=r.choice([1,2,3,8,32,256]);data=bytes(r.randrange(alphabet) for _ in range(n))
 cases.append((data,r.choice([0,0,0,1]),r.choice([0,1,2,3])))
cases += [(b'A'*10000,0,1),(b'A'*10000,0,3),(bytes(range(256))*8,0,2),(b'ABC'*5000,0,3),(b'A'*8000+b'B'*8000,0,3),(b'AB'*1000,256,1),(b'AB'*1000,257,1)]
compressed=0;max_passes=0
for i,(data,bypass,passes) in enumerate(cases):
 a=native(data,bypass=bypass,passes=passes);b=portable(data,bypass=bypass,passes=passes)
 assert a==b,(i,len(a),len(b),next((j for j,(x,y) in enumerate(zip(a,b)) if x!=y),None))
 na=native(a,True);pa=portable(a,True);assert na==pa,(i,len(na),len(pa))
 compressed+=int(a[:4]!=b'\0'*4);max_passes=max(max_passes,int.from_bytes(a[:4],'big'))
 if len(set(data))>1 or a[:4]==b'\0'*4:assert pa==data,i
assert max_passes>=2,max_passes
assert portable(b'')==b'\0'*4
for malformed in (b'',b'\0',b'\0\0\0\1',b'\0\0\0\1'+bytes(768)+b'\0\0\0\1'+b'\x80'):
 out=C.c_int32();assert not lib.sniper_network_decompress(malformed,len(malformed),C.byref(out))
print(f'PASS {len(cases)} native compression and decompression cases; {compressed} compressed streams; stable ties, multiple passes, bypass, incompressible and native single-symbol data loss; malformed rejection')
