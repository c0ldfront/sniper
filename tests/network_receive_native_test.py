"""Original 401a9d control dispatch; OS clock/send/close intercepted.
ACK dispatch delegates to separately recovered sender; native ACK storage tested there.
"""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/src/network_receive.c'),'-o',str(root/'code/build/libnetwork_receive.so')],check=True)
class Slot(C.Structure):_fields_=[('sequence',C.c_int32),('data',C.c_uint8*512),('length',C.c_int32)]
class State(C.Structure):_fields_=[(n,C.c_int32) for n in ['replies','best_rtt','clock_offset','expected','closed']]+[('slots',Slot*16),('bundle',C.c_uint8*1024),('bundle_length',C.c_int32),('bundle_cursor',C.c_int32),('fragment',C.c_void_p),('completed',C.c_void_p),('fragment_length',C.c_size_t),('completed_length',C.c_size_t)]
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p);Send=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p,C.c_size_t);Ack=C.CFUNCTYPE(None,C.c_void_p,C.c_uint16,C.c_uint32);Close=C.CFUNCTYPE(None,C.c_void_p)
class Host(C.Structure):_fields_=[('user',C.c_void_p),('clock',Clock),('send',Send),('ack',Ack),('close',Close),('decompress',C.c_void_p)]
lib=C.CDLL(str(root/'code/build/libnetwork_receive.so'));lib.sniper_network_receive_control.argtypes=[C.POINTER(State),C.c_void_p,C.c_size_t,C.POINTER(Host)]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
ce=[];ne=[];tick=0
@Clock
def clock(_):return tick
@Send
def send(_,p,n):ce.append(('send',C.string_at(p,n)))
@Ack
def ack(_,i,s):ce.append(('ack',i,s))
@Close
def close(_):ce.append(('close',))
h=Host(None,clock,send,ack,close,None)
def hook(uc,a,z,user):
    if a==0x401dad:uc.emu_stop();return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x700000:ret=tick;cleanup=0
    elif a==0x41614c:ne.append(('send',bytes(uc.mem_read(get(sp+8),get(sp+12)))));ret=0;cleanup=16
    elif a==0x41611c:ne.append(('close',));ret=0;cleanup=4
    else:return
    uc.reg_write(UC_X86_REG_EAX,ret);uc.reg_write(UC_X86_REG_ESP,sp+4+cleanup);uc.reg_write(UC_X86_REG_EIP,get(sp))
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x700000)
r=random.Random(1987)
for case in range(1800):
    s=State();s.replies=r.randrange(20);s.best_rtt=r.choice([-100,0,1,100,2147483647]);s.clock_offset=r.randrange(-1000,1000);s.expected=r.choice([1,2,1000,2147483647]);tick=r.randrange(2**32)
    for q in s.slots:
        q.sequence=r.choice([0,0,0,s.expected,s.expected+1 if s.expected<2147483647 else -2147483648]);q.length=r.randrange(20)
        q.data[:]=r.randbytes(512)
    typ=r.choice([0,1,2,2,2,4,7,8,255]);seq=r.choice([0,s.expected,((s.expected+1)&0xffffffff),r.randrange(1000)])
    if typ==0:p=b'\0\0'+struct.pack('<I',seq)
    elif typ==1:p=b'\0\1'+struct.pack('<II',seq,r.randrange(2**32))
    elif typ==2:p=b'\0\2'+struct.pack('<IH',seq,r.randrange(256))+r.randbytes(r.randrange(505))
    elif typ==7:p=b'\0\7'+r.randbytes(r.randrange(100))
    else:p=bytes([0,typ])
    base=0x500000;u.mem_write(base,bytes(0x4000));put(base+0x10,s.replies);put(base+4,s.best_rtt);put(base+0x8ac,s.clock_offset);put(base+0x3c54,s.expected);put(base+0x8b0,3)
    u.mem_write(base+0x1bd4,bytes(s.slots));u.mem_write(0x600000,p);put(0x610000,len(p));put(0x800038,0x610000)
    ce.clear();ne.clear();u.reg_write(UC_X86_REG_EBX,base);u.reg_write(UC_X86_REG_EBP,0x600000);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x401a9d,0x401daf,count=10000)
    assert lib.sniper_network_receive_control(C.byref(s),p,len(p),C.byref(h))==1
    assert ce==ne,(case,typ,ce,ne)
    for field,off in [('replies',0x10),('best_rtt',4),('clock_offset',0x8ac),('expected',0x3c54),('bundle_length',0x820),('bundle_cursor',0x824)]:assert getattr(s,field)==get(base+off),(case,typ,field,getattr(s,field),get(base+off))
    assert bytes(s.slots)==bytes(u.mem_read(base+0x1bd4,C.sizeof(s.slots))),(case,typ)
    assert bytes(s.bundle)==bytes(u.mem_read(base+0x420,1024)),case
    assert s.closed==(get(base+0x8b0)==-1)
print('PASS 1800 native receive control cases: ping/pong timing, reliable queue/drop/ACK, close, bundle, unknown controls')
# Native pop selection: bundle and reliable slots; stop before packet dispatch.
lib.sniper_network_receive_pop.argtypes=[C.POINTER(State),C.POINTER(C.c_size_t)];lib.sniper_network_receive_pop.restype=C.c_void_p
for case in range(600):
    s=State();s.expected=r.randrange(1,2000)
    for i,q in enumerate(s.slots):q.sequence=s.expected if i==case%17 else 0;q.length=r.randrange(1,100);q.data[:]=r.randbytes(512)
    if case%3==0:
        payload=b'\x40'+r.randbytes(r.randrange(80));bundle=b'\0\7'+bytes([len(payload)])+payload;s.bundle[:len(bundle)]=bundle;s.bundle_length=len(bundle);s.bundle_cursor=2
    base=0x500000;u.mem_write(base,bytes(0x4000));u.mem_write(base+0x1bd4,bytes(s.slots));put(base+0x3c54,s.expected);u.mem_write(base+0x420,bytes(s.bundle));put(base+0x820,s.bundle_length);put(base+0x824,s.bundle_cursor);put(0x800038,0x610000);put(0x610000,0)
    # Force recv peek failure for queue-only path.
    def pophook(uc,a,z,user):
        if a==0x401a9d:uc.emu_stop();return
        if a==0x416152:
            sp=uc.reg_read(UC_X86_REG_ESP);uc.reg_write(UC_X86_REG_EAX,0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+20);uc.reg_write(UC_X86_REG_EIP,get(sp))
    ph=u.hook_add(UC_HOOK_CODE,pophook)
    u.reg_write(UC_X86_REG_EFLAGS,2)
    u.reg_write(UC_X86_REG_EBX,base);u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4019e4,0x401a9d,count=10000);u.hook_del(ph)
    ptr=u.reg_read(UC_X86_REG_EBP);n=C.c_size_t();cp=lib.sniper_network_receive_pop(C.byref(s),C.byref(n))
    assert bool(cp)==bool(ptr),(case,cp,ptr)
    if cp:assert n.value==get(0x610000) and C.string_at(cp,n.value)==bytes(u.mem_read(ptr,n.value)),case
    assert s.expected==get(base+0x3c54) and s.bundle_cursor==get(base+0x824)
    assert bytes(s.slots)==bytes(u.mem_read(base+0x1bd4,C.sizeof(s.slots)))
print('PASS 600 native bundle/reliable pop selections and state transitions')
# Fragment5/6 control transitions; allocator and codec boundaries intercepted.
# Codec deliberately returns reversed bytes: this verifies ownership/control,
# not the separately recovered compression algorithm.
Decode=C.CFUNCTYPE(C.c_void_p,C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t))
libc=C.CDLL(None);libc.malloc.argtypes=[C.c_size_t];libc.malloc.restype=C.c_void_p
@Decode
def decode(_,p,n,out):
    data=C.string_at(p,n)[::-1];out[0]=n;b=libc.malloc(max(1,n));C.memmove(b,data,n);return b
h.decompress=C.cast(decode,C.c_void_p).value
lib.sniper_network_receive_clear.argtypes=[C.POINTER(State)]
for case in range(600):
    s=State();old=r.randbytes(r.randrange(1,100));s.fragment=libc.malloc(len(old));C.memmove(s.fragment,old,len(old));s.fragment_length=len(old)
    typ=r.choice([5,6]);tail=b'' if case%2 else r.randbytes(r.randrange(1,100));p=bytes([0,typ])+tail
    base=0x500000;u.mem_write(base,bytes(0x4000));u.mem_write(0x620000,old);put(base+0xbb8,0x620000);put(base+0xbbc,len(old));u.mem_write(0x600000,p);put(0x610000,len(p));put(0x800038,0x610000)
    def frag_hook(uc,a,z,user):
        sp=uc.reg_read(UC_X86_REG_ESP)
        if a==0x416210:result=0x620000
        elif a==0x410c60:
            data=bytes(uc.mem_read(get(sp+4),get(sp+8)))[::-1];uc.mem_write(0x630000,data);put(get(sp+12),len(data));result=0x630000
        else:return
        uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4);uc.reg_write(UC_X86_REG_EIP,get(sp))
    fh=u.hook_add(UC_HOOK_CODE,frag_hook);u.reg_write(UC_X86_REG_EBX,base);u.reg_write(UC_X86_REG_EBP,0x600000);u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x401a9d,0x401daf,count=10000);u.hook_del(fh)
    assert lib.sniper_network_receive_control(C.byref(s),p,len(p),C.byref(h))==1
    for field,off,length in [('fragment',0xbb8,'fragment_length'),('completed',0xbc0,'completed_length')]:
        ptr=get(base+off);actual=getattr(s,field);n=getattr(s,length);assert bool(ptr)==bool(actual)
        assert n==get(base+off+4)
        if ptr:assert C.string_at(actual,n)==bytes(u.mem_read(ptr,n))
    lib.sniper_network_receive_clear(C.byref(s))
print('PASS 600 native fragment append/finalize cases, raw and controlled compressed paths')
# Deliberate safety adaptations: no original execution for malformed inputs.
s=State();s.expected=1000
for packet in [b'',b'\0',b'\0\0',b'\0\1'+b'x'*7,b'\0\2'+b'x'*5,b'\0\2'+b'x'*519,b'\0\3'+b'\0'*4+b'\xff\xff',b'\0\7'+b'x'*1023]:
    assert lib.sniper_network_receive_control(C.byref(s),packet,len(packet),C.byref(h))==-1
for packet in [b'\0\7\xff',b'\0\7\x05a']:
    assert lib.sniper_network_receive_control(C.byref(s),packet,len(packet),C.byref(h))==1
    n=C.c_size_t();assert not lib.sniper_network_receive_pop(C.byref(s),C.byref(n))
lib.sniper_network_receive_clear(C.byref(s))
print('PASS malformed-input safety guards (explicit native divergence)')
