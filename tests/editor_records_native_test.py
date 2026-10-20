"""Original 40a3e0 mutations and full 411580 backend vs portable records/map.
Only CRT realloc/memmove intercepted. Original GUI is never launched.
"""
import ctypes as C
import pathlib, random, struct, subprocess, tempfile
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ECX
root=pathlib.Path(__file__).resolve().parents[2]
tmp=tempfile.TemporaryDirectory()
so=pathlib.Path(tmp.name)/'editor.so'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-O2',str(root/'code/src/editor_records.c'),str(root/'code/src/map.c'),'-o',str(so)],check=True)
lib=C.CDLL(str(so))
class Records(C.Structure):
    _fields_=[('records',C.POINTER(C.c_uint32)),('count',C.c_size_t),('checksum',C.c_uint32),('dirty',C.c_int)]
class Size(C.Structure): _fields_=[('width',C.c_int32),('height',C.c_int32)]
lib.sniper_editor_records_load.argtypes=[C.POINTER(Records),C.c_void_p,C.c_size_t]
lib.sniper_editor_records_place.argtypes=[C.POINTER(Records),C.c_int32,C.c_int32,C.c_int32]
lib.sniper_editor_records_bytes.argtypes=[C.POINTER(Records),C.c_void_p,C.c_size_t]
lib.sniper_editor_records_bytes.restype=C.c_size_t
lib.sniper_editor_records_free.argtypes=[C.POINTER(Records)]
lib.sniper_map_place.argtypes=[C.c_void_p,C.POINTER(Size),C.c_uint,C.c_uint,C.c_uint8]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x600000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def hook(u,a,n,ctx):
    if a not in (0x416210,0x416280):return
    sp=u.reg_read(UC_X86_REG_ESP);ret=get(sp)
    if a==0x416210:
        assert get(sp+8)<0x10000
        u.reg_write(UC_X86_REG_EAX,0x600000)
    else:
        dst,src,n=[get(sp+i) for i in (4,8,12)]
        if n:u.mem_write(dst,bytes(u.mem_read(src,n)))
        u.reg_write(UC_X86_REG_EAX,dst)
    u.reg_write(UC_X86_REG_ESP,sp+4);u.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87);total=0
for full in (False,True):
 for case in range(150):
    xy=[(r.randrange(50),r.randrange(50)) for _ in range(8)]
    words=[x|(y<<12)|(r.randrange(256)<<24) for x,y in xy+xy[:3]]
    data=b''.join(struct.pack('<I',v) for v in words)+bytes(r.randrange(256) for _ in range(case%4))
    s=Records();buf=C.create_string_buffer(data);assert lib.sniper_editor_records_load(C.byref(s),buf,len(data))
    assert s.checksum==sum(data[:len(words)*4]);assert s.dirty==0
    u.mem_write(0x600000,data[:len(words)*4]);put(0x500100,0);put(0x500104,0x600000);put(0x500108,len(words));put(0x50010c,s.checksum);put(0x510004,0x500000)
    grid=(C.c_uint8*1048576)();sizes=(Size*256)()
    if full:
        put(0x429f48,0x700000);u.mem_write(0x700000,bytes(grid))
        for i in range(216,256):
            sizes[i]=Size(r.choice([16,32,48]),r.choice([16,32,48]))
            ptr=0x520000+i*0x100;put(0x42da18+i*4,ptr);put(ptr+0x84,sizes[i].width);put(ptr+0x88,sizes[i].height)
    for step in range(20):
        x,y=r.choice(xy) if step%3 else (r.randrange(50),r.randrange(50))
        tile=r.choice([0,0,1,152,216,218,250,255])
        if not full and step%7==0:x=r.choice([-1,4096,8191]);tile=r.choice([0,256,-1,512])
        sp=0x900000;put(sp,0x900100)
        for off,v in [(4,x),(8,y),(12,tile)]:put(sp+off,v)
        u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,0x510000 if full else 0x500000)
        u.emu_start(0x411580 if full else 0x40a3e0,0x900100,count=100000)
        if full:assert lib.sniper_map_place(grid,sizes,x,y,tile)==0
        assert lib.sniper_editor_records_place(C.byref(s),x,y,tile)==1
        output=(C.c_uint8*(s.count*4))();assert lib.sniper_editor_records_bytes(C.byref(s),output,len(output))==len(output)
        assert s.count==get(0x500108) and bytes(output)==bytes(u.mem_read(0x600000,s.count*4)),(full,case,step)
        assert s.dirty==get(0x500100)==1 and s.checksum==get(0x50010c)
        if full:assert bytes(grid)==bytes(u.mem_read(0x700000,1048576)),(case,step)
        total+=1
    lib.sniper_editor_records_free(C.byref(s))
print(f'PASS {total} native record mutations, including 3000 complete offline map-placement calls; duplicate ordering, deletion, truncation, dirty/checksum and large-tile footprints')
# Constructor/destructor CRT boundary traces, including load trailing-byte rule.
io={}
def iohook(u,a,n,ctx):
    if a not in (0x4165c0,0x41f890,0x416b30,0x416190,0x416440,0x4163d0,0x417d80,0x417c00,0x416170):return
    sp=u.reg_read(UC_X86_REG_ESP);args=[get(sp+i) for i in (4,8,12,16)];ret=get(sp);v=0
    io['calls'].append(a)
    if a==0x4165c0:
        mode=bytes(u.mem_read(args[1],3));assert mode in (b'rb\0',b'wb\0')
        io['mode']=mode;v=0 if io.get('fail') else 0x550000
    elif a==0x41f890:v=7
    elif a==0x416b30:v=len(io['data'])
    elif a==0x416190:v=0x600000;assert args[0]==len(io['data'])//4*4
    elif a==0x416440:
        assert args[1:3]==[len(io['data'])//4,4]
        payload=io['data'][:args[1]*args[2]]
        if payload:u.mem_write(args[0],payload)
        v=4
    elif a==0x417c00:
        io['written']=bytes(u.mem_read(args[0],args[1]*args[2]));v=4
    u.reg_write(UC_X86_REG_EAX,v);u.reg_write(UC_X86_REG_ESP,sp+4);u.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,iohook)
for n in range(100):
    io.clear();io.update(data=bytes(r.randrange(256) for _ in range(n)),calls=[],fail=n%11==0)
    u.mem_write(0x530000,b'fixture.lvl\0');put(0x900000,0x900100);put(0x900004,0x530000)
    u.reg_write(UC_X86_REG_ESP,0x900000);u.reg_write(UC_X86_REG_ECX,0x500000)
    u.emu_start(0x40a290,0x900100,count=10000)
    count=0 if io['fail'] else n//4
    assert get(0x500108)==count and get(0x50010c)==sum(io['data'][:count*4]) and get(0x500100)==0
for dirty in (0,1):
 for pointer in (0,0x600000):
  for fail in (False,True):
    io.clear();io.update(calls=[],fail=fail)
    put(0x500100,dirty);put(0x500104,pointer);put(0x500108,2)
    u.mem_write(0x600000,b'12345678');put(0x900000,0x900100)
    u.reg_write(UC_X86_REG_ESP,0x900000);u.reg_write(UC_X86_REG_ECX,0x500000)
    u.emu_start(0x40a370,0x900100,count=10000)
    expected=[]
    if dirty and pointer:
        expected=[0x417d80,0x4165c0]
        if not fail:expected +=[0x417c00,0x4163d0];assert io['written']==b'12345678'
    if pointer:expected +=[0x416170]
    assert io['calls']==expected,(dirty,pointer,fail,io)
print('PASS 100 original constructor load/checksum cases and 8 original destructor save/free traces (CRT I/O mocked)')
