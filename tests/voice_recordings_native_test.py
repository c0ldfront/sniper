"""Original startup file loading plus actual410920 compression, filesystem seams."""
import ctypes as C,pathlib,random,struct,tempfile
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_EBP
root=pathlib.Path(__file__).resolve().parents[2]
class Recordings(C.Structure):_fields_=[('data',C.c_void_p*2),('length',C.c_int32*2)]
lib=C.CDLL(str(root/'code/build/libvoice_recordings.so'))
lib.sniper_voice_recordings_load.argtypes=[C.POINTER(Recordings),C.c_char_p]
lib.sniper_voice_recordings_close.argtypes=[C.POINTER(Recordings)]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_map(0x1000000,0x4000000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def put(a,n):u.mem_write(a,struct.pack('<I',n))
allocs={};heap=0x1000000

def allocation(uc,a,sz,_):
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
u.hook_add(UC_HOOK_CODE,allocation)
def string(a):
 out=b''
 while (c:=bytes(u.mem_read(a,1)))!=b'\0':out+=c;a+=1
 return out
opened=[];closed=[];compressions=[]
def files(uc,a,sz,_):
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a==0x410920:
  assert get(sp+16)==0 and get(sp+20)==2
  compressions.append(bytes(u.mem_read(get(sp+4),get(sp+8))));return
 if a not in (0x4165c0,0x41f890,0x416b30,0x416440,0x4163d0):return
 if a==0x4165c0:
  name=string(get(sp+4));assert string(get(sp+8))==b'rb'
  assert name in [b'record0.wav',b'record1.wav'];slot=int(name[6:7]);opened.append(slot)
  result=0x600100+slot if inputs[slot] is not None else 0
 elif a==0x41f890:result=get(sp+4)
 elif a==0x416b30:result=len(inputs[get(sp+4)-0x600100])
 elif a==0x416440:
  raw=inputs[get(sp+16)-0x600100];assert get(sp+8)==1 and get(sp+12)==len(raw)
  if raw:u.mem_write(get(sp+4),raw)
  result=len(raw)
 else:closed.append(get(sp+4)-0x600100);result=0
 uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4);uc.reg_write(UC_X86_REG_EIP,get(sp))
u.hook_add(UC_HOOK_CODE,files)
r=random.Random(4034)
for case in range(24):
 inputs=[r.choice([None,b'not a WAV',b'RIFF'+b'ABCD'*1000,r.randbytes(1234)]) for _ in range(2)]
 opened.clear();closed.clear();compressions.clear();heap=0x1000000;allocs.clear()
 u.mem_write(0x428320,b'\x5a'*16);u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EBP,0x810000)
 u.emu_start(0x4034b5,0x403569,count=30000000)
 assert u.reg_read(UC_X86_REG_EIP)==0x403569
 assert opened==[0,1] and closed==[i for i,x in enumerate(inputs) if x is not None]
 assert compressions==[x for x in inputs if x is not None]
 with tempfile.TemporaryDirectory() as tmp:
  for i,data in enumerate(inputs):
   if data is not None:pathlib.Path(tmp,f'record{i}.wav').write_bytes(data)
  out=Recordings();lib.sniper_voice_recordings_load(C.byref(out),tmp.encode())
  before=[]
  for i in range(2):
   ptr=get(0x428320+8*i);size=get(0x428324+8*i)
   assert bool(out.data[i])==bool(ptr) and out.length[i]==size
   expected=bytes(u.mem_read(ptr,size)) if ptr else None
   actual=C.string_at(out.data[i],out.length[i]) if out.data[i] else None
   assert actual==expected,(case,i);before.append(actual)
  # Replacing a recording on disk does not mutate the loaded snapshot.
  pathlib.Path(tmp,'record0.wav').write_bytes(b'new recording')
  assert (C.string_at(out.data[0],out.length[0]) if out.data[0] else None)==before[0]
  lib.sniper_voice_recordings_close(C.byref(out));assert not any(out.data) and list(out.length)==[0,0]
print('PASS24 native recording-load pairs: exact filenames/order, raw bytes, real2-pass compression, missing files, ownership and snapshot lifetime')

# Original empty file reaches compression's NULL Huffman-root dereference.
from unicorn import UcError
inputs=[b'',None];heap=0x1000000;allocs.clear()
u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EBP,0x810000)
try:u.emu_start(0x4034b5,0x403569,count=30000000)
except UcError:assert u.reg_read(UC_X86_REG_EIP)==0x4109f6
else:raise AssertionError('Expected original empty-input tree failure')
with tempfile.TemporaryDirectory() as tmp:
 pathlib.Path(tmp,'record0.wav').write_bytes(b'')
 out=Recordings();lib.sniper_voice_recordings_load(C.byref(out),tmp.encode())
 assert out.length[0]==4 and C.string_at(out.data[0],4)==bytes(4)
 lib.sniper_voice_recordings_close(C.byref(out))
print('Verified native empty-file crash; portable codec deliberately emits empty4-byte envelope')
