"""Native40ad50 chunk scan and copies; only heap allocator intercepted."""
import ctypes as C,pathlib,struct,random,subprocess
import pefile
import original_resources
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-shared','-fPIC','-O2',str(root/'code/src/audio_logic.c'),'-o',str(root/'code/build/libaudio_wave.so')],check=True)
lib=C.CDLL(str(root/'code/build/libaudio_wave.so'))
class Wave(C.Structure):_fields_=[('format',C.c_ubyte*18),('pcm',C.c_void_p),('bytes',C.c_uint32),('has_format',C.c_int)]
lib.sniper_audio_wave.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(Wave)]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,n):u.mem_write(a,struct.pack('<I',n))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
allocations=[]
def hook(uc,a,size,user):
 if a!=0x416190:return
 sp=uc.reg_read(UC_X86_REG_ESP);allocations.append(get(sp+4));uc.reg_write(UC_X86_REG_EAX,0x600000+len(allocations)*0x10000);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
def chunk(tag,data):return tag+struct.pack('<I',len(data))+data+(b'\x00' if len(data)%2 else b'')
fixtures=[d for k,d in sorted(original_resources.payloads().items(),key=lambda kv:kv[0]+'.bin') if k.startswith('WAVE-')]
for n in range(1200):
 chunks=[]
 for i in range(r.randrange(1,5)):
  chunks += [chunk(b'fmt ',r.randbytes(r.choice([16,18,20]))),chunk(b'data',r.randbytes(r.randrange(1,400))),chunk(b'JUNK',r.randbytes(r.randrange(1,20)))]
 r.shuffle(chunks)
 # Native's18-byte fmt read remains in input when fmt16 is final.
 body=b'WAVE'+b''.join(chunks);fixtures.append(b'RIFF'+struct.pack('<I',len(body))+body+b'TRAILING EXTRA BYTES'+chunk(b'data',b'wrong'))
for case,raw in enumerate(fixtures):
 allocations.clear();u.mem_write(0x500000,raw);u.mem_write(0x550100,bytes(18));put(0x550000,0)
 for at,n in [(0x800000,0x540000),(0x800004,0x500000),(0x800008,0x550000),(0x80000c,0x550100)]:put(at,n)
 u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x40ad50,0x540000,count=1000000)
 native=u.reg_read(UC_X86_REG_EAX);data=C.create_string_buffer(raw);wave=Wave();assert lib.sniper_audio_wave(data,len(raw),C.byref(wave)),case
 assert bytes(wave.format)==bytes(u.mem_read(0x550100,18)),case
 assert wave.bytes==get(0x550000),case
 assert C.string_at(wave.pcm,wave.bytes)==bytes(u.mem_read(native,wave.bytes)),case
for raw in [b'',b'RIFF'+struct.pack('<I',100)+b'WAVE',b'NOTRIFFWAVE',b'RIFF'+struct.pack('<I',5)+b'WAVEx']:
 data=C.create_string_buffer(raw);assert not lib.sniper_audio_wave(data,len(raw),C.byref(Wave()))
print(f'PASS{len(fixtures)} native WAVE parser cases:original resources,padded/unknown/reordered/repeated chunks,RIFF boundary,18-byte format copies;4 malformed inputs rejected safely')
