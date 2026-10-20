"""Complete409b10, actual CRT atoi and offline backend wrappers; downstream hooks."""
import ctypes as C,pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Notices(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
Send=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_char_p)
Event=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
class Dispatch(C.Structure):_fields_=[('local',C.POINTER(Ship)),('ordered',C.POINTER(C.POINTER(Ship))),('count',C.c_int32),('selected',C.c_int32),('modulus',C.c_int32),('capacity',C.c_int32),('requested',C.POINTER(C.c_int32)),('notices',C.POINTER(Notices)),('send',Send),('team',Event),('attach',Event),('user',C.c_void_p)]
lib=C.CDLL(str(root/'code/build/libchat_dispatch.so'));lib.sniper_chat_dispatch.argtypes=[C.POINTER(Dispatch),C.c_char_p]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def string(a):
    out=b''
    while (v:=bytes(u.mem_read(a,1)))!=b'\0':out+=v;a+=1
    return out
trace=[];portable=[];mutation=None
@Send
def send(_,recipient,text):portable.append(('send',recipient,text))
@Event
def team(_,frequency):
    portable.append(('team',frequency,requested.value))
    if mutation is not None:ships[0].word[27]=mutation
@Event
def attach(_,target):portable.append(('attach',target))
def hook(uc,a,size,user):
    if a not in (0x404c50,0x404530,0x404fe0):return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x404c50:
        assert get(sp+8)==0
        trace.append(('send',get(sp+12),string(get(sp+4))))
    elif a==0x404530:
        assert get(sp+4)==0
        trace.append(('team',get(sp+8),get(0x42def0)))
        if mutation is not None:put(0x50006c,mutation)
    else:
        assert get(sp+4)==0
        trace.append(('attach',get(sp+8)))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
for a,v in [(0x4218e0,0x500000),(0x424348,0x520000),(0x520000,0x420008)]:put(a,v)
r=random.Random(409);kinds=set()
messages=[b'',b'   ',b' hello ',b'\tpublic',b"'team",b"'",b'//team',b'//',b'/private',b'/',b'///slash',b'=0',b'=1',b'=-1',b'= +123junk',b'=\t-2',b'=none',b'=4294967295',b'=-2147483648',b'=999999999999999999999999999999',b'=\v12',b' =3']
for case in range(1800):
    ships=(Ship*8)();ordered=(C.POINTER(Ship)*8)(*[C.pointer(s) for s in ships]);n=Notices()
    count=r.randrange(9);selected=r.randrange(10);modulus=r.choice([1,2,4,13,-7,2147483647]);capacity=r.randrange(-1,6)
    for i,s in enumerate(ships):
        s.word[17]=r.randrange(1024);s.word[26]=r.randrange(-3,4);s.word[27]=r.choice([-1,0,2]);put(0x4283e8+4*i,0x500000+536*i)
    mutation=r.choice([None,-1,7]);requested=C.c_int32(1234)
    C.memset(C.addressof(n),0x5a,C.sizeof(n))
    for i in range(4):n.ticks[i]=r.choice([-3,0,1,400])
    text=b' '*(case%4)+messages[case%len(messages)]
    if case%23==0:text=b'//'+b'x'*250
    u.mem_write(0x500000,bytes(ships));u.mem_write(0x530000,text+b'\0')
    for a,v in [(0x429328,count),(0x429f58,selected),(0x4282a9,modulus),(0x4282b1,capacity),(0x42def0,1234),(0x800000,0x800100),(0x800004,0x530000)]:put(a,v)
    for a,b in [(0x424330,bytes(n.ticks)),(0x4287e8,bytes(n.text)),(0x428a50,bytes(n.color))]:u.mem_write(a,b)
    trace.clear();portable.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x409b10,0x800100,count=100000)
    d=Dispatch(C.pointer(ships[0]),ordered,count,selected,modulus,capacity,C.pointer(requested),C.pointer(n),send,team,attach,None)
    lib.sniper_chat_dispatch(C.byref(d),text)
    assert trace==portable,(case,text,trace,portable)
    assert requested.value==get(0x42def0)
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships)))
    for a,b in [(0x424330,bytes(n.ticks)),(0x4287e8,bytes(n.text)),(0x428a50,bytes(n.color))]:assert bytes(u.mem_read(a,len(b)))==b,(case,hex(a))
    kinds.update(t[0] for t in trace)
assert kinds=={'send','team','attach'},kinds
print('PASS1800 complete native chat dispatch cases: actual CRT parse and offline wrappers, routes, team capacity, post-team attachment, notices and callback ordering')
