"""Original 401610 send + 401e00 pump; socket/clock/allocation boundaries only.
Large compression is a controlled callback (codec tested separately)."""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/src/network_send.c'),'-o',str(root/'code/build/libnetwork_send.so')],check=True)
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p);Send=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_void_p,C.c_int32)
class Pending(C.Structure):_fields_=[('data',C.c_void_p),('size',C.c_int32),('seq',C.c_uint32),('tick',C.c_uint32)]
class Large(C.Structure):_fields_=[('data',C.c_void_p),('size',C.c_int32),('offset',C.c_int32),('compressed',C.c_int32)]
class State(C.Structure):_fields_=[('connected',C.c_int32),('closing',C.c_int32),('tick',C.c_uint32),('minimum',C.c_int32),('sent',C.c_int32),('received',C.c_int32),('threshold',C.c_int32),('bundlesize',C.c_int32),('bundle',C.c_uint8*1024),('seq',C.c_uint32),('head',C.c_int32),('count',C.c_int32),('largecount',C.c_int32),('pumping',C.c_int32),('pending',Pending*256),('large',Large*16),('bytes',C.c_uint32),('packets',C.c_uint32),('clock',Clock),('send',Send),('compress',C.c_void_p),('user',C.c_void_p)]
lib=C.CDLL(str(root/'code/build/libnetwork_send.so'));lib.sniper_network_send_packet.argtypes=[C.POINTER(State),C.c_void_p,C.c_int32,C.c_int,C.c_int];lib.sniper_network_send_pump.argtypes=[C.POINTER(State)];lib.sniper_network_send_close.argtypes=[C.POINTER(State)];lib.sniper_network_send_ack.argtypes=[C.POINTER(State),C.c_uint16,C.c_uint32]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x600000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
base=0x500000;trace=[];portable=[];clocktick=0;ptick=0;step=0;failure=0;heap=0x600000
@Clock
def clock(_):
    global ptick
    v=ptick;ptick=(ptick+step)&0xffffffff;return v
@Send
def send(_,p,n):portable.append(C.string_at(p,n));return n-failure

def hook(uc,a,size,user):
    global heap,clocktick
    if a not in (0x41614c,0x416190,0x416170,0x416280,0x900000):return
    sp=uc.reg_read(UC_X86_REG_ESP);ret=get(sp);pop=4;value=0
    if a==0x900000:value=(clocktick*10)&0xffffffff;clocktick=(clocktick+step)&0xffffffff
    elif a==0x41614c:
        n=get(sp+12);trace.append(bytes(uc.mem_read(get(sp+8),n)));value=n-failure;pop+=16
    elif a==0x416190:value=heap;heap+=(get(sp+4)+31)&~15
    elif a==0x416280:uc.mem_write(get(sp+4),bytes(uc.mem_read(get(sp+8),get(sp+12))))
    uc.reg_write(UC_X86_REG_EAX,value&0xffffffff);uc.reg_write(UC_X86_REG_EIP,ret);uc.reg_write(UC_X86_REG_ESP,sp+pop)
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x900000)
allocator=C.CDLL(None);allocator.malloc.argtypes=[C.c_size_t];allocator.malloc.restype=C.c_void_p
r=random.Random(401610)
for case in range(900):
    s=State();lib.sniper_network_send_init(C.byref(s));s.connected=case%11!=0;s.closing=r.randrange(2);s.clock=clock;s.send=send
    s.tick=r.randrange(80000);s.sent=r.randrange(25);s.received=r.randrange(15);s.threshold=r.choice([0,32,64,128,200]);s.head=r.randrange(256);s.seq=r.randrange(0xffffffff)
    u.mem_write(base,b'\0'*0x4000);heap=0x600000;put(base+0x8b0,3 if s.connected else -1)
    for off,v in [(0,s.tick),(4,s.minimum),(8,s.sent),(12,s.closing),(16,s.received),(24,s.threshold),(0x1bc8,s.seq),(0x1bcc,s.head)]:put(base+off,v)
    s.count=r.choice([0,1,13,14,31,32,255,256]);put(base+0x1bd0,s.count)
    for relative in range(s.count):
        i=(s.head+relative)%256;q=s.pending[i];q.data=allocator.malloc(8);assert q.data;q.size=8;q.seq=relative+1000;q.tick=r.randrange(100000)
        packet=b'\0\2'+struct.pack('<IH',q.seq,i);C.memmove(q.data,packet,8);u.mem_write(heap,packet)
        for off,v in [(0,q.seq),(4,heap),(8,8),(12,q.tick)]:put(base+0xbc8+16*i+off,v)
        heap+=16
    put(0x424308,0);put(0x424310,0);put(0x421034,0)
    for turn in range(8):
        n=r.choice([0,1,15,31,64,127,200,499,500,501,960,1500]);p=r.randbytes(n);reliable=r.randrange(2);step=r.randrange(3);clocktick=ptick=r.randrange(100000);failure=r.choice([0,0,1]);trace.clear();portable.clear()
        u.mem_write(0x580000,p or b'\0');sp=0x880000
        for off,v in [(0,0x880100),(4,0x580000),(8,n),(12,reliable),(16,0)]:put(sp+off,v)
        u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,base);u.emu_start(0x401610,0x880100,count=200000)
        expected=u.reg_read(UC_X86_REG_EAX)&255;got=lib.sniper_network_send_packet(C.byref(s),p,n,reliable,0)
        assert expected==got,(case,turn,'return',expected,got)
        assert trace==portable,(case,turn,'packets',trace,portable)
        assert ptick==clocktick,(case,turn,'clock',ptick,clocktick)
        for off,v in [(0,s.tick),(4,s.minimum),(8,s.sent),(12,s.closing),(16,s.received),(24,s.threshold),(28,s.bundlesize),(0x1bc8,s.seq),(0x1bcc,s.head),(0x1bd0,s.count),(0xbb4,s.largecount)]:assert get(base+off)==(v&0xffffffff),(case,turn,hex(off),get(base+off),v)
        assert get(0x424308)==s.bytes and get(0x424310)==s.packets
        assert bytes(u.mem_read(base+32,1024))==bytes(s.bundle)
        for i,q in enumerate(s.pending):
            a=base+0xbc8+16*i
            assert (get(a),get(a+8),get(a+12))==(q.seq,q.size,q.tick),(case,turn,i,'pending metadata')
            assert bool(get(a+4))==bool(q.data)
            if q.data:assert bytes(u.mem_read(get(a+4),q.size))==C.string_at(q.data,q.size)
        for i in range(s.largecount):
            q=s.large[i];a=base+0xab4+16*i
            assert (get(a+4),get(a+8),get(a+12))==(q.size,q.offset,q.compressed)
            assert bytes(u.mem_read(get(a),q.size))==C.string_at(q.data,q.size)
        for ack_turn in range(3):
            i=(s.head+r.randrange(max(s.count,1)))%256;seq=s.pending[i].seq
            if ack_turn==2:seq^=0x1234
            u.mem_write(0x590000,b'\0\3'+struct.pack('<IH',seq,i));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EBX,base);u.reg_write(UC_X86_REG_EBP,0x590000)
            u.emu_start(0x401c25,0x401dad,count=20000);lib.sniper_network_send_ack(C.byref(s),i,seq)
            assert get(base+0x1bcc)==s.head and get(base+0x1bd0)==s.count
            for j,q in enumerate(s.pending):assert bool(get(base+0xbcc+16*j))==bool(q.data)
    lib.sniper_network_send_close(C.byref(s))
print('PASS7200 native send/pump transitions: ping sampling, unreliable bundling/drop/error, reliable bytes/ring/retry and large uncompressed fragmentation plus21600 ACK transitions')
