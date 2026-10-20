"""Native401fa0 file checksum; only file CRT functions intercepted."""
import ctypes as C,pathlib,random,struct,subprocess,tempfile
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/src/network_checksum.c'),'-o',str(root/'code/build/libnetwork_checksum.so')],check=True)
lib=C.CDLL(str(root/'code/build/libnetwork_checksum.so'));lib.sniper_network_checksum.argtypes=[C.c_void_p,C.c_size_t];lib.sniper_network_checksum.restype=C.c_uint32
lib.sniper_network_checksum_file.argtypes=[C.c_char_p];lib.sniper_network_checksum_file.restype=C.c_uint32
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def put(a,n):u.mem_write(a,struct.pack('<I',n))
data=b'';offset=0;opened=True;closed=0
def hook(uc,a,sz,_):
 global offset,closed
 if a not in (0x4165c0,0x416440,0x4163d0):return
 sp=uc.reg_read(UC_X86_REG_ESP);result=0
 if a==0x4165c0:
  assert bytes(u.mem_read(get(sp+8),3))==b'rb\0';result=0x610000 if opened else 0
 elif a==0x416440:
  assert [get(sp+i) for i in (8,12,16)]==[1,4,0x610000]
  block=data[offset:offset+4];offset+=len(block);result=len(block)
  if block:u.mem_write(get(sp+4),block)
  if len(block)<4:put(0x61000c,0x10)
 else:closed+=1
 uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(600):
 data=bytes(r.randrange(256) for _ in range(case if case<20 else r.randrange(9000)));offset=closed=0;opened=case%17!=0;put(0x61000c,0)
 u.mem_write(0x800000,struct.pack('<II',0x700000,0x600000));u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x401fa0,0x700000,count=100000)
 expected=lib.sniper_network_checksum(data,len(data)) if opened else 0
 assert u.reg_read(UC_X86_REG_EAX)==expected,case
 assert closed==int(opened)
 with tempfile.NamedTemporaryFile() as f:
  f.write(data);f.flush();assert lib.sniper_network_checksum_file(f.name.encode())==lib.sniper_network_checksum(data,len(data))
assert lib.sniper_network_checksum_file(b'/nonexistent/sniper87-checksum-test')==0
for path in (root/'code/build/assets/files/SNIPER.EXE',root/'code/build/assets/files/LOCAL.LVL'):
 b=path.read_bytes();print(path.name,hex(lib.sniper_network_checksum(b,len(b))))
print('PASS600 native file checksums: unsigned wrap, little endian,0..3-byte tails, missing file; portable file adapter matches buffer')
