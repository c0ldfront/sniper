"""Native login builders/reply extraction/join/cache-name slices.
UI prompts, transport send, and memmove are intercepted where required.
"""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/src/network_login.c'),'-o',str(root/'code/build/libnetwork_login.so')],check=True)
class Packet(C.Structure):_fields_=[('data',C.c_uint8*512),('length',C.c_uint32),('reliable',C.c_uint32),('copy',C.c_uint32)]
class Reply(C.Structure):_fields_=[('status',C.c_uint8),('version',C.c_int32),('map',C.c_uint32),('exe',C.c_uint32),('arena',C.c_uint32)]
lib=C.CDLL(str(root/'code/build/libnetwork_login.so'));lib.sniper_login_packet.argtypes=[C.POINTER(Packet),C.c_char_p,C.c_char_p,C.c_int];lib.sniper_login_reply.argtypes=[C.POINTER(Reply),C.c_void_p,C.c_size_t];lib.sniper_login_join_packet.argtypes=[C.POINTER(Packet)]+[C.c_int32]*5;lib.sniper_login_map_name.argtypes=[C.c_void_p,C.c_char_p,C.c_char_p];lib.sniper_login_accepted.argtypes=[C.POINTER(Reply)];lib.sniper_login_version_matches.argtypes=[C.POINTER(Reply),C.c_uint32]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
sent=[]
def hook(uc,a,z,user):
    if a==0x401610:
        sp=uc.reg_read(UC_X86_REG_ESP);sent.append((bytes(uc.mem_read(get(sp+4),get(sp+8))),get(sp+12),get(sp+16)));uc.emu_stop()
    elif a==0x416280:
        sp=uc.reg_read(UC_X86_REG_ESP);uc.mem_write(get(sp+4),bytes(uc.mem_read(get(sp+8),get(sp+12))));uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(1000):
    name=bytes(r.choice(b'abcXYZ019') for _ in range(r.randrange(32)));password=bytes(r.choice(b'abcXYZ019') for _ in range(r.randrange(32)))
    u.mem_write(0x42e178,name+b'\0');u.mem_write(0x42e158,password+b'\0');u.mem_write(0x7ff000,bytes(0x2000));u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000);u.emu_start(0x40b297,0x40b31f,count=10000)
    p=Packet();assert lib.sniper_login_packet(C.byref(p),name,password,0)
    assert bytes(p.data[:98])==bytes(u.mem_read(0x800000-0x13c,98));assert (p.length,p.reliable,p.copy)==(98,1,1)
    args=[r.randrange(-2**31,2**31) for _ in range(5)]
    for off,v in zip([0xc,0x14,0x18,0x1c,0x20],args):put(0x800000+off,v)
    u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000);u.reg_write(UC_X86_REG_ESI,0x500000);sent.clear();u.emu_start(0x4116fc,0x411744,count=10000)
    lib.sniper_login_join_packet(C.byref(p),*args);assert sent==[(bytes(p.data[:p.length]),p.reliable,p.copy)]
    wire=b'\x0a'+bytes([r.randrange(256)])+r.randbytes(16);reply=Reply();assert lib.sniper_login_reply(C.byref(reply),wire,len(wire))==1
    u.mem_write(0x600000,wire);u.reg_write(UC_X86_REG_EAX,0x600000);u.reg_write(UC_X86_REG_EBP,0x800000);u.emu_start(0x40b386,0x40b3a4,count=10000)
    assert (reply.status,reply.version&0xffffffff,reply.map,reply.exe,reply.arena)==(u.reg_read(UC_X86_REG_EAX)&255,get(0x422298),get(0x7ffff0),u.reg_read(UC_X86_REG_ESI),get(0x800000-0x240))
    assert lib.sniper_login_accepted(C.byref(reply))==(wire[1] not in (1,2,3))
    host=bytes(r.choice(b'abcdef019.') for _ in range(r.randrange(1,80)));port=str(r.randrange(65536)).encode();u.mem_write(0x42def8,host+b'\0');u.mem_write(0x42e150,port+b'\0');u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000);u.emu_start(0x40b5ad,0x40b6b3,count=100000)
    # final rep movsb is after0x40b6b3, so execute those instructions too.
    u.emu_start(0x40b6b3,0x40b6bd,count=10000)
    out=C.create_string_buffer(13);assert lib.sniper_login_map_name(out,host,port);assert out.value==bytes(u.mem_read(0x42e008,13)).split(b'\0')[0]
print('PASS 1000 native login builds, join packets, reply field extraction and map cache filenames; deterministic zero-padding adaptation')
# Native status/version branches, stopping at UI decisions.
for status in range(256):
    reply=Reply(status,87,0,0,0);u.reg_write(UC_X86_REG_EAX,status)
    reached=[]
    def decision(uc,a,z,user):
        if a in (0x40b3a8,0x40b3ed,0x40b424,0x40b457):reached.append(a);uc.emu_stop()
    hk=u.hook_add(UC_HOOK_CODE,decision);u.emu_start(0x40b3a4,0x40b459,count=10000);u.hook_del(hk)
    assert bool(lib.sniper_login_accepted(C.byref(reply)))==(reached==[0x40b457])
for case in range(500):
    version=r.choice([-2147483648,-1,0,86,87,88,2147483647]);expected=r.randrange(2**32);local=r.choice([expected,r.randrange(2**32)]);reply=Reply(0,version,0,expected,0)
    put(0x422298,version);put(0x42df78,local);u.reg_write(UC_X86_REG_ESI,expected);reached=[]
    def version_branch(uc,a,z,user):
        if a in (0x40b4fd,0x40b5a0):reached.append(a);uc.emu_stop()
    hk=u.hook_add(UC_HOOK_CODE,version_branch);u.emu_start(0x40b4e8,0x40b5ad,count=10000);u.hook_del(hk)
    assert bool(lib.sniper_login_version_matches(C.byref(reply),local))==(reached==[0x40b5a0])
print('PASS 256 native login status branches and500 native version/checksum decisions')
class Download(C.Structure):_fields_=[('offset',C.c_int32),('total',C.c_int32),('done',C.c_int32)]
Write=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_void_p,C.c_size_t)
lib.sniper_login_download.argtypes=[C.POINTER(Download),C.c_void_p,C.c_size_t,Write,C.c_void_p]
ce=[];ne=[];native_offset=0;native_done=False
@Write
def write(_,offset,p,n):ce.append((offset,C.string_at(p,n)))
for import_addr,stub in [(0x4353ac,0x700100),(0x4353bc,0x700110),(0x435510,0x700120),(0x435560,0x700130)]:put(import_addr,stub)
def download_hook(uc,a,z,user):
    global native_offset,native_done
    if a==0x40f7ee:native_done=True;uc.emu_stop();return
    if a==0x40f7e4:uc.emu_stop();return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x700100:native_offset=get(sp+8);cleanup=16
    elif a==0x700110:ne.append((native_offset,bytes(uc.mem_read(get(sp+8),get(sp+12)))));cleanup=20
    elif a==0x700120:cleanup=8
    elif a==0x700130:cleanup=16
    else:return
    uc.reg_write(UC_X86_REG_EAX,1);uc.reg_write(UC_X86_REG_ESP,sp+4+cleanup);uc.reg_write(UC_X86_REG_EIP,get(sp))
hk=u.hook_add(UC_HOOK_CODE,download_hook)
for case in range(500):
    offset=r.choice([-1,0,r.randrange(2**31)]);packet=b'\x0c'+struct.pack('<i',offset)+r.randbytes(r.randrange(300));s=Download();ce.clear();ne.clear();native_done=False
    assert lib.sniper_login_download(C.byref(s),packet,len(packet),write,None)==1
    u.mem_write(0x600000,packet);u.reg_write(UC_X86_REG_EDI,0x600000);u.reg_write(UC_X86_REG_ESI,123);u.reg_write(UC_X86_REG_ESP,0x800000);put(0x800014,len(packet));u.emu_start(0x40f742,0x40f7fb,count=10000)
    assert s.done==native_done and ce==[(C.c_int32(a).value,b) for a,b in ne]
u.hook_del(hk)
print('PASS 500 native download write-offset/payload and completion transitions')
for entry,name in [(0x40b77f,'sniper_login_map_request'),(0x40b904,'sniper_login_arena_request'),(0x40f6a0,'sniper_login_cancel_download')]:
    p=Packet();getattr(lib,name)(C.byref(p));sent.clear();u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000);u.emu_start(entry,0x40f800,count=10000);assert sent==[(bytes(p.data[:1]),1,1)]
assert not lib.sniper_login_packet(C.byref(Packet()),b'a'*32,b'pw',0)
assert lib.sniper_login_reply(C.byref(Reply()),b'\x0a\x00',2)==-1
print('PASS native map/arena/cancel request encoders and malformed credential/reply guards')
