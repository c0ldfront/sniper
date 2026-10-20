"""Full409180 input body; OS keys, allocation and backend map-place are controlled."""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-O2',str(root/'code/src/editor_input.c'),'-o',str(root/'code/build/libeditor_input.so')],check=True)
class Editor(C.Structure):_fields_=[(k,C.c_int32) for k in ['x','y','tile','selection','x1','y1','x2','y2','clipboard_width','clipboard_height']]+[('clipboard',C.c_void_p)]
class Input(C.Structure):_fields_=[(k,C.c_int32) for k in ['key','repeat','radar','radar_dirty']]+[('character',C.c_uint8)]
class Size(C.Structure):_fields_=[('width',C.c_int32),('height',C.c_int32)]
Key=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_int32);Place=C.CFUNCTYPE(None,C.c_void_p,C.c_int32,C.c_int32,C.c_int32);Exit=C.CFUNCTYPE(None,C.c_void_p)
class Host(C.Structure):_fields_=[('async_key',Key),('event_key',Key),('place',Place),('exit',Exit),('user',C.c_void_p)]
lib=C.CDLL(str(root/'code/build/libeditor_input.so'));lib.sniper_editor_input.argtypes=[C.POINTER(Editor),C.POINTER(Input),C.c_void_p,C.POINTER(Size),C.POINTER(Host)];lib.sniper_editor_close.argtypes=[C.POINTER(Editor)]
libc=C.CDLL(None);libc.malloc.restype=C.c_void_p;libc.malloc.argtypes=[C.c_size_t]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
fields=[0x4218d4,0x4218d8,0x4218dc,0x429324,0x4283dc,0x429f4c,0x4281bc,0x428a6c,0x428a04,0x4281a0]
put(0x429f48,0x600000);put(0x424348,0x500000);put(0x500000,0x500100);put(0x500110,0x530030)
for a,v in [(0x4354b8,0x530000),(0x4354c0,0x530010),(0x4354d0,0x530020)]:put(a,v)
sizes=(Size*256)()
for tile in range(216,220):
 sizes[tile]=Size((tile-214)*16,(220-tile)*16);put(0x42da18+tile*4,0x510000+(tile-216)*256);put(0x510084+(tile-216)*256,sizes[tile].width);put(0x510088+(tile-216)*256,sizes[tile].height)
trace=[];expected=[];keys={};shift=0
@Key
def async_key(_,k):trace.append(('async',k));return keys.get(k,0)
@Key
def event_key(_,k):trace.append(('event',k));return shift
@Place
def place(_,x,y,t):
 trace.append(('place',x,y,t))
 if 0<=x<1024 and 0<=y<1024:grid[y*1024+x]=t&255
@Exit
def exit(_):trace.append(('exit',))
host=Host(async_key,event_key,place,exit,None)
def hook(uc,a,size,user):
 if a not in [0x530000,0x530010,0x530020,0x530030,0x416190,0x416170]:return
 sp=uc.reg_read(UC_X86_REG_ESP);clean=0;result=0
 if a in [0x530000,0x530010]:
  k=get(sp+4);expected.append(('async' if a==0x530000 else 'event',k));result=keys.get(k,0) if a==0x530000 else shift;clean=4
 elif a==0x530020:expected.append(('exit',));clean=16
 elif a==0x530030:
  x,y,t=[get(sp+j) for j in (4,8,12)];expected.append(('place',x,y,t));clean=12
  if 0<=x<1024 and 0<=y<1024:uc.mem_write(0x600000+y*1024+x,bytes([t&255]))
 elif a==0x416190:result=0x720000
 uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87180);grid=(C.c_uint8*1048576)();blank=bytes(1048576);total=0
for case in range(2400):
 C.memset(grid,0,len(grid));e=Editor();e.x=r.choice([0,1,25,1021,1023]);e.y=r.choice([0,1,31,1021,1023]);e.tile=r.choice([1,19,152]);e.selection=case%3==0
 e.x1=max(0,e.x-r.randrange(4));e.y1=max(0,e.y-r.randrange(4));e.x2=e.x;e.y2=e.y
 for yy in range(max(0,e.y-4),min(1024,e.y+5)):
  for xx in range(max(0,e.x-4),min(1024,e.x+5)):grid[yy*1024+xx]=r.choice([0,0,0,1,152,216,250,251,255])
 if case%7==0:C.memset(grid,0,len(grid))
 if case%3==1:
  e.clipboard_width=r.randrange(1,5);e.clipboard_height=r.randrange(1,5);clip=bytes(r.choice([0,1,216,249,250,255]) for _ in range(e.clipboard_width*e.clipboard_height));e.clipboard=libc.malloc(len(clip));C.memmove(e.clipboard,clip,len(clip));u.mem_write(0x720000,clip)
 i=Input(r.choice([0,13,27,32,37,38,39,40,46,115,116]),r.choice([0,0x40000000]),r.choice([-1,0,1,2]),r.randrange(2),r.choice([0,ord('e'),ord('p'),ord('s'),ord('A'),ord('D'),255]))
 keys={k:r.choice([0,0,0x8000]) for k in [17,37,38,39,40]};shift=r.choice([0,0x8000])
 for (name,_),a in zip(Editor._fields_,fields):put(a,getattr(e,name))
 put(0x4281ac,0x720000 if e.clipboard else 0)
 for a,v in [(0x422248,i.key),(0x42224c,i.repeat),(0x429308,i.radar),(0x428334,i.radar_dirty)]:put(a,v)
 u.mem_write(0x422244,bytes([i.character]));u.mem_write(0x600000,bytes(grid));put(0x800000,0x530100);u.reg_write(UC_X86_REG_ESP,0x800000);expected.clear();trace.clear()
 u.emu_start(0x409180,0x530100,count=1000000)
 assert lib.sniper_editor_input(C.byref(e),C.byref(i),grid,sizes,C.byref(host)),case
 assert trace==expected,(case,trace,expected)
 for (name,_),a in zip(Editor._fields_,fields):assert getattr(e,name)==get(a),(case,name,getattr(e,name),get(a))
 assert bool(e.clipboard)==bool(get(0x4281ac)),case
 if e.clipboard:assert C.string_at(e.clipboard,e.clipboard_width*e.clipboard_height)==bytes(u.mem_read(get(0x4281ac),e.clipboard_width*e.clipboard_height)),case
 assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),case
 for name,a in [('key',0x422248),('repeat',0x42224c),('radar',0x429308),('radar_dirty',0x428334)]:assert getattr(i,name)==get(a),(case,name)
 total+=sum(t[0]=='place' for t in trace);lib.sniper_editor_close(C.byref(e))
print(f'PASS2400 full original editor-input cases; {total} map mutation callbacks, clipboard and complete key/exit traces match')
