"""Complete411d00 queue dispatch; gameplay, CRT file operations and transport intercepted."""
import ctypes as C,pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class State(C.Structure):_fields_=[('player_id',C.c_int32),('ready',C.c_int32),('offset',C.c_uint32),('settings',C.c_uint8*289)]
class Event(C.Structure):_fields_=[('kind',C.c_uint32),('count',C.c_uint32),('args',C.c_int32*15),('data',C.c_void_p),('data2',C.c_void_p),('size',C.c_size_t),('size2',C.c_size_t)]
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p);Emit=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Event));Next=C.CFUNCTYPE(C.c_void_p,C.c_void_p,C.POINTER(C.c_size_t))
class Host(C.Structure):_fields_=[('user',C.c_void_p),('clock',Clock),('event',Emit),('next',Next)]
lib=C.CDLL(str(root/'code/build/libnetwork_game_receive.so'));lib.sniper_game_receive_drain.argtypes=[C.POINTER(State),C.POINTER(Host)]
lib.sniper_game_receive_packet.argtypes=[C.POINTER(State),C.POINTER(Host),C.c_void_p,C.c_size_t]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def signed(x):return x if x<2**31 else x-2**32
def string(a):
 out=bytearray()
 while True:
  b=u.mem_read(a,1)[0];out.append(b);a+=1
  if not b:return bytes(out)
def ret(clean=0):
 sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,get(sp));u.reg_write(UC_X86_REG_ESP,sp+4+clean)
counts={0x404340:10,0x404600:1,0x404a60:15,0x404680:4,0x404c50:3,0x404910:4,0x404f10:4,0x404090:3,0x404530:2,0x404fe0:2,0x4165c0:2,0x417c00:4,0x4163d0:1}
ne=[];ce=[];queue=[];idx=0;cidx=0;ms=0;mutate=False

def hook(uc,a,size,user):
 global idx
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a==0x401920:
  assert uc.reg_read(UC_X86_REG_ECX)==0x510000
  ne.append(('poll',signed(get(0x500008)),signed(get(0x50000c))))
  if idx<len(queue):
   packet=queue[idx];addr=0x610000+idx*0x1000;idx+=1;uc.mem_write(addr,packet);put(get(sp+4),len(packet));uc.reg_write(UC_X86_REG_EAX,addr)
  else:uc.reg_write(UC_X86_REG_EAX,0)
  ret(4);return
 if a==0x600000:ne.append(('clock',));uc.reg_write(UC_X86_REG_EAX,ms);ret();return
 if a==0x416bc0:
  putaddr=get(sp+4);msg=b'File received: '+string(get(sp+12));uc.mem_write(putaddr,msg);ret();return
 if a not in counts:return
 args=[signed(get(sp+4+4*i)) for i in range(counts[a])];d=b'';d2=b''
 if a==0x404340:
  d=bytes(uc.mem_read(args[1],32));d2=bytes(uc.mem_read(args[2],32));args[1]=args[2]=0
 elif a==0x404c50:d=string(args[0]);args[0]=0
 elif a==0x404090:d=bytes(uc.mem_read(args[1],args[2]));args[1]=0
 elif a==0x4165c0:d=string(args[0]);d2=string(args[1]);args=[0,0];uc.reg_write(UC_X86_REG_EAX,0x650000)
 elif a==0x417c00:
  assert args[3]==0x650000;d=bytes(uc.mem_read(args[0],args[1]));args[0]=args[3]=0
 elif a==0x4163d0:assert args[0]==0x650000;args[0]=0
 ne.append((a,tuple(args),d,d2))
 if mutate and a==0x4163d0:put(0x500008,241)
 if mutate and a==0x404340:put(0x50000c,1)
 ret()
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x600000);put(0x500010,0x510000)
@Clock
def clock(_):ce.append(('clock',));return ms
@Emit
def emit(_,ep):
 e=ep.contents;ce.append((e.kind,tuple(e.args[:e.count]),C.string_at(e.data,e.size) if e.size else b'',C.string_at(e.data2,e.size2) if e.size2 else b''))
 if mutate and e.kind==0x4163d0:state.player_id=241
 if mutate and e.kind==0x404340:state.ready=1
buffers=[]
@Next
def next_packet(_,length):
 global cidx
 ce.append(('poll',state.player_id,state.ready))
 if cidx==len(buffers):return None
 b=buffers[cidx];length[0]=len(queue[cidx]);cidx+=1;return C.addressof(b)
host=Host(None,clock,emit,next_packet);r=random.Random(8705)
minimum={1:2,2:1,3:85,4:2,5:20,6:6,7:5,8:8,9:10,14:2,15:9,16:5,17:289,18:17}
packet_count=0;seen=set()
for case in range(1600):
 state=State(r.randrange(-10,300),r.choice([0,1,-1]),r.randrange(2**32),(C.c_uint8*289)(*[r.randrange(256) for _ in range(289)]))
 put(0x500008,state.player_id);put(0x50000c,state.ready);put(0x5108ac,state.offset);u.mem_write(0x4281c8,bytes(state.settings));ms=r.randrange(2**32);mutate=case%3==0
 ops=list(range(1,19))+[0,19,127,128,255] if case<20 else [r.randrange(256) if r.randrange(5)==0 else r.randrange(1,19) for _ in range(r.randrange(1,30))]
 r.shuffle(ops);queue=[]
 for op in ops:
  n=minimum.get(op,1);p=bytearray(r.randrange(256) for _ in range(n));p[0]=op
  if op==7:p=p[:4]+b'chat-test-'+bytes([65+case%26])+b'\0'
  if op==14:p+=bytes(r.randrange(256) for _ in range(r.randrange(200)))
  if op==18:p[1:17]=b'level87.lvl\0'.ljust(16,b'\0');p+=bytes(r.randrange(256) for _ in range(r.randrange(100)))
  if op==5 and case%4==0:
   now=(ms//10+state.offset)&0xffffffff;now%=0x19999999
   timestamp=(now-r.choice([0,1,79,80,81,0x80000000,0xffffffff]))&0xffffffff;p[11:15]=struct.pack('<I',timestamp)
  queue.append(bytes(p));seen.add(op)
 buffers=[C.create_string_buffer(p) for p in queue];idx=cidx=0;ne.clear();ce.clear();put(0x800000,0x700000)
 u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000);u.emu_start(0x411d00,0x700000,count=200000)
 lib.sniper_game_receive_drain(C.byref(state),C.byref(host))
 assert ce==ne,(case,ce,ne)
 assert state.player_id==signed(get(0x500008)) and state.ready==signed(get(0x50000c)),case
 assert bytes(state.settings)==bytes(u.mem_read(0x4281c8,289)),case
 packet_count+=len(queue)
# Portable safety-only tests: native performs unchecked reads on malformed data.
for op,n in minimum.items():
 state.ready=1
 for size in range(1,n):
  p=bytes([op])+bytes(size-1);assert lib.sniper_game_receive_packet(C.byref(state),C.byref(host),p,len(p))==0,(op,size)
assert set(range(1,19)).issubset(seen)
print(f'PASS1600 full original dispatcher queues / {packet_count} packets: all opcodes, exact args/events/order, settings, ready/id, timestamp wrap and live callback mutation')
