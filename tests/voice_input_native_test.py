"""Shift gate4052cd through voice branch, actual401600 transfer queue getter."""
import ctypes as C,pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Notices(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
Send=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_void_p,C.c_int32)
class Input(C.Structure):_fields_=[('ordered',C.POINTER(C.POINTER(Ship))),('count',C.c_int32),('selected',C.c_int32),('enabled',C.c_int32),('present',C.c_int32),('queued',C.c_int32),('recordings',C.c_void_p*2),('lengths',C.c_int32*2),('notices',C.POINTER(Notices)),('send',Send),('user',C.c_void_p)]
lib=C.CDLL(str(root/'code/build/libvoice_input.so'));lib.sniper_voice_input.argtypes=[C.POINTER(Input),C.c_int32,C.c_int32]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
trace=[];portable=[];queries=[]
@Send
def send(_,recipient,slot,data,length):portable.append((recipient,slot,data or 0,length))
def hook(uc,a,size,user):
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x401600:queries.append(1);return
    if a==0x510000:assert get(sp+4)==16;uc.reg_write(UC_X86_REG_EAX,shift&0xffffffff);extra=4
    elif a==0x510010:trace.append(tuple(get(sp+i) for i in [4,8,12,16]));extra=16
    else:return
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+extra)
u.hook_add(UC_HOOK_CODE,hook)
for a,v in [(0x4354c0,0x510000),(0x424348,0x520000),(0x520000,0x520100),(0x520124,0x510010)]:put(a,v)
r=random.Random(405360);sent=0
for case in range(1800):
    ships=(Ship*8)();ordered=(C.POINTER(Ship)*8)(*[C.pointer(s) for s in ships]);notices=Notices()
    count=r.randrange(9);selected=r.randrange(10);enabled=r.choice([0,1,-1]);present=r.randrange(2);queued=r.choice([0,0,1,-1,100]);shift=r.choice([0,1,0x8000,-32768]);key=r.choice([0x79,0x7a,0x7b,0x7c,-1])
    for i,s in enumerate(ships):s.word[17]=r.randrange(1024);s.word[28]=r.choice([0,1,-1]);put(0x4283e8+4*i,0x500000+536*i)
    C.memset(C.addressof(notices),0x5a,C.sizeof(notices))
    for i in range(4):notices.ticks[i]=r.choice([-1,0,1,500])
    data=[r.choice([0,0x550000,0x551000]),r.choice([0,0x552000,0x553000])];lengths=[r.choice([0,-1,1,50000]),r.choice([0,-1,1,60000])]
    u.mem_write(0x500000,bytes(ships))
    for a,v in [(0x429328,count),(0x429f58,selected),(0x4282e1,enabled),(0x422238,0x530000 if present else 0),(0x530bb4,queued),(0x422248,key),(0x42224c,123),(0x428320,data[0]),(0x428324,lengths[0]),(0x428328,data[1]),(0x42832c,lengths[1])]:put(a,v)
    for a,b in [(0x424330,bytes(notices.ticks)),(0x4287e8,bytes(notices.text)),(0x428a50,bytes(notices.color))]:u.mem_write(a,b)
    trace.clear();portable.clear();queries.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4052cd,0x40549a,count=10000)
    c=Input(ordered,count,selected,enabled,present,queued,(C.c_void_p*2)(*data),(C.c_int32*2)(*lengths),C.pointer(notices),send,None)
    consumed=lib.sniper_voice_input(C.byref(c),key,shift)
    assert trace==portable,(case,trace,portable)
    assert get(0x422248)==(0 if consumed else key)
    assert get(0x42224c)==(0 if consumed else 123)
    for a,b in [(0x424330,bytes(notices.ticks)),(0x4287e8,bytes(notices.text)),(0x428a50,bytes(notices.color))]:assert bytes(u.mem_read(a,len(b)))==b,case
    expected_query=bool(shift&0x8000 and key in [0x7a,0x7b] and enabled and selected<count and ships[selected].word[28] and present)
    assert bool(queries)==expected_query,case
    sent+=len(trace)
assert sent>10,sent
print(f'PASS1800 native voice-input cases,{sent} sends: Shift gate, target permission, real large-queue getter, recording selection, notice and key consumption')
