"""Entire original401920 receive loop, actual401e00 maintenance (empty outbound).
Only socket I/O and clock replaced; compares multi-call ordered delivery/counters.
"""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',*[str(root/'code/src'/f'network_{n}.c') for n in ['send','receive','transport']],'-o',str(root/'code/build/libnetwork_transport.so')],check=True)
class Pending(C.Structure):_fields_=[('data',C.c_void_p),('size',C.c_int32),('sequence',C.c_uint32),('last_sent',C.c_uint32)]
class Large(C.Structure):_fields_=[('data',C.c_void_p),('size',C.c_int32),('offset',C.c_int32),('compressed',C.c_int32)]
class Sender(C.Structure):_fields_=[('connected',C.c_int32),('closing',C.c_int32),('ping_tick',C.c_uint32)]+[(n,C.c_int32) for n in ['ping_min','ping_sent','ping_received','bundle_threshold','bundle_size']]+[('bundle',C.c_uint8*1024),('next_sequence',C.c_uint32)]+[(n,C.c_int32) for n in ['head','count','large_count','pumping']]+[('pending',Pending*256),('large',Large*16),('bytes_sent',C.c_uint32),('packets_sent',C.c_uint32)]+[(n,C.c_void_p) for n in ['clock','send','compress','user']]
class Slot(C.Structure):_fields_=[('sequence',C.c_int32),('data',C.c_uint8*512),('length',C.c_int32)]
class Receiver(C.Structure):_fields_=[(n,C.c_int32) for n in ['replies','best_rtt','clock_offset','expected','closed']]+[('slots',Slot*16),('bundle',C.c_uint8*1024),('bundle_length',C.c_int32),('bundle_cursor',C.c_int32),('fragment',C.c_void_p),('completed',C.c_void_p),('fragment_length',C.c_size_t),('completed_length',C.c_size_t)]
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p);Read=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_void_p,C.c_int32,C.c_int);Send=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_void_p,C.c_int32);Close=C.CFUNCTYPE(None,C.c_void_p)
class Host(C.Structure):_fields_=[('user',C.c_void_p),('clock',Clock),('read',Read),('send',Send),('close',Close),('compress',C.c_void_p),('decompress',C.c_void_p)]
class Transport(C.Structure):_fields_=[('sender',Sender),('receiver',Receiver),('host',Host),('buffer',C.c_uint8*512)]+[(n,C.c_uint32) for n in ['last_receive_tick','bytes_received','packets_received']]+[('invalid_packet',C.c_int)]
lib=C.CDLL(str(root/'code/build/libnetwork_transport.so'));lib.sniper_network_transport_init.argtypes=[C.POINTER(Transport),C.POINTER(Host)];lib.sniper_network_transport_receive.argtypes=[C.POINTER(Transport),C.POINTER(C.c_size_t)];lib.sniper_network_transport_receive.restype=C.c_void_p;lib.sniper_network_transport_clear.argtypes=[C.POINTER(Transport)]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
stream=[];cursor=0;tick=0;events=[];clock_calls=0
@Clock
def clock(_):
    global clock_calls
    v=(tick+clock_calls*13)&0xffffffff;clock_calls+=1;return v
@Read
def read(_,p,n,peek):
    global cursor
    if cursor==len(stream):events.append(('read',n,peek,-1));return -1
    data=stream[cursor][:n];C.memmove(p,data,len(data));events.append(('read',n,peek,len(data)))
    if not peek:cursor+=1
    return len(data)
@Send
def send(_,p,n):events.append(('send',C.string_at(p,n)));return n
@Close
def close(_):events.append(('close',))
h=Host(None,clock,read,send,close,None,None)
def hook(uc,a,z,user):
    global cursor
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x700000:ret=clock(None);cleanup=0
    elif a==0x41614c:
        n=get(sp+12);events.append(('send',bytes(uc.mem_read(get(sp+8),n))));ret=n;cleanup=16
    elif a==0x416152:
        n=get(sp+12);peek=int(get(sp+16)==2)
        if cursor==len(stream):ret=-1
        else:
            data=stream[cursor][:n];ret=len(data);uc.mem_write(get(sp+8),data)
            if not peek:cursor+=1
        events.append(('read',n,peek,ret));cleanup=16
    elif a==0x41611c:events.append(('close',));ret=0;cleanup=4
    else:return
    uc.reg_write(UC_X86_REG_EAX,ret&0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+4+cleanup);uc.reg_write(UC_X86_REG_EIP,get(sp))
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x700000)
r=random.Random(401920);delivered=0
for case in range(500):
    tick=r.randrange(2**32);stream=[]
    seqs=list(range(1000,1000+r.randrange(1,12)));r.shuffle(seqs)
    for seq in seqs:
        app=b'\x23'+r.randbytes(r.randrange(1,50));stream.append(b'\0\2'+struct.pack('<IH',seq,r.randrange(256))+app)
        if r.randrange(3)==0:stream.append(b'\0\0'+r.randbytes(4))
        if r.randrange(3)==0:stream.append(b'\0\1'+r.randbytes(8))
        if r.randrange(3)==0:
            a=b'\x34'+r.randbytes(r.randrange(20));stream.append(b'\0\7'+bytes([len(a)])+a)
    if case%5==0:stream.append(b'\0\4')
    clock_calls=0;t=Transport();lib.sniper_network_transport_init(C.byref(t),C.byref(h));t.sender.ping_sent=r.choice([0,15,16,20]);t.sender.ping_received=r.choice([0,9,10]);t.sender.ping_tick=r.randrange(2**32)
    base=0x500000;u.mem_write(base,bytes(0x4000));put(base,t.sender.ping_tick);put(base+4,9999999);put(base+8,t.sender.ping_sent);put(base+0x10,t.sender.ping_received);put(base+0x8b0,3);put(base+0x3c54,1000);put(base+0x14,t.last_receive_tick)
    for a in [0x424300,0x424304,0x424308,0x424310]:put(a,0)
    nc=cc=0;nt=ct=1
    for call in range(30):
        events.clear();cursor=nc;clock_calls=nt;put(0x800000,0x710000);put(0x800004,0x610000);u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,base);u.emu_start(0x401920,0x710000,count=200000)
        ptr=u.reg_read(UC_X86_REG_EAX);native=bytes(u.mem_read(ptr,get(0x610000))) if ptr else None;ne=list(events);nc=cursor;nt=clock_calls
        events.clear();cursor=cc;clock_calls=ct;n=C.c_size_t();p=lib.sniper_network_transport_receive(C.byref(t),C.byref(n));actual=C.string_at(p,n.value) if p else None;cc=cursor;ct=clock_calls
        assert native==actual,(case,call,native,actual)
        assert ne==events,(case,call,ne,events)
        assert nc==cc and nt==ct,(case,call,nc,cc,nt,ct)
        for value,off in [(t.sender.ping_tick,0),(t.sender.ping_min,4),(t.sender.ping_sent,8),(t.sender.ping_received,0x10),(t.last_receive_tick,0x14),(t.receiver.clock_offset,0x8ac),(t.receiver.expected,0x3c54)]:assert value&0xffffffff==get(base+off),(case,call,off,value,get(base+off))
        for value,a in [(t.bytes_received,0x424304),(t.packets_received,0x424300),(t.sender.bytes_sent,0x424308),(t.sender.packets_sent,0x424310)]:assert value==get(a),(case,call,a,value,get(a))
        if actual is None:break
        delivered+=1
    lib.sniper_network_transport_clear(C.byref(t))
print(f'PASS 500 whole native receive traces: {delivered} deliveries; socket peek/consume priority, reliable ordering, bundle nesting, ping samples, pong clocks, close and counters')
