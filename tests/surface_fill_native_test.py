"""Original415e50/415c00/415c40 pixel buffers and surface lifecycle."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
STACK,PIX,OBJ,VT,RECTADDR,BLT,LOCK,UNLOCK,RESTORE,STOP=[0x1000000+i*65536 for i in range(10)]
for a in (STACK,PIX,OBJ,VT,RECTADDR,BLT,LOCK,UNLOCK,RESTORE,STOP):u.mem_map(a,65536)
def word(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def words(a,n):return struct.unpack('<'+'I'*n,u.mem_read(a,n*4))
word(OBJ,VT)
for offset,fn in [(0x14,BLT),(0x64,LOCK),(0x80,UNLOCK),(0x6c,RESTORE)]:word(VT+offset,fn)
class Rect(C.Structure):_fields_=[(n,C.c_int32) for n in ('left','top','right','bottom')]
class Buffer(C.Structure):_fields_=[('pixels',C.POINTER(C.c_uint8))]+[(n,C.c_int32) for n in ('pitch','width','height')]
Blt=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.POINTER(Rect),C.c_uint32)
Lock=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.POINTER(Buffer))
Unlock=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.POINTER(C.c_uint8))
Restore=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
class Surface(C.Structure):_fields_=[('blt',Blt),('lock',Lock),('unlock',Unlock),('restore',Restore),('user',C.c_void_p)]
lib=C.CDLL(str(ROOT/'code/build/libsurface_fill.so'))
lib.sniper_surface_fill.argtypes=[C.POINTER(Surface),C.c_int,C.c_int32,C.POINTER(Rect),C.c_uint32];lib.sniper_surface_fill.restype=C.c_uint32
for name in ('vertical','horizontal'):
    f=getattr(lib,'sniper_surface_'+name);f.argtypes=[C.POINTER(Surface),C.c_int]+[C.c_int32]*4+[C.c_uint32];f.restype=C.c_uint32
native=[];actual=[];results=[];nidx=cidx=0;lock_result=unlock_result=restore_result=0
pixels=(C.c_uint8*65536)()
@Blt
def blt(_,rect,color):
    global cidx
    actual.append(('blt',tuple(getattr(rect.contents,n) for n in ('left','top','right','bottom')),color));v=results[cidx];cidx+=1;return v
@Lock
def lock(_,out):
    actual.append(('lock',));out[0]=Buffer(pixels,80,64,64);return lock_result
@Unlock
def unlock(_,ptr):actual.append(('unlock',));return unlock_result
@Restore
def restore(_):actual.append(('restore',));return restore_result
surface=Surface(blt,lock,unlock,restore,None)
def intercept(uc,a,size,user):
    global nidx
    if a not in (BLT,LOCK,UNLOCK,RESTORE):return
    sp=uc.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0]
    if a==BLT:
        obj,rect,source,srect,flags,fx=words(sp+4,6);assert (obj,source,srect,flags)==(OBJ,0,0,0x400)
        raw=bytearray(uc.mem_read(fx,100));assert words(fx,1)[0]==100
        color=words(fx+80,1)[0];raw[:4]=b'\0'*4;raw[80:84]=b'\0'*4;assert not any(raw)
        native.append(('blt',struct.unpack('<4i',uc.mem_read(rect,16)),color));result=results[nidx];nidx+=1;count=6
    elif a==LOCK:
        obj,rect,desc,flags,event=words(sp+4,5);assert (obj,rect,flags,event)==(OBJ,0,1,0)
        word(desc+16,80);word(desc+36,PIX);native.append(('lock',));result=lock_result;count=5
    elif a==UNLOCK:
        assert words(sp+4,2)==(OBJ,PIX);native.append(('unlock',));result=unlock_result;count=2
    else:
        assert words(sp+4,1)==(OBJ,);native.append(('restore',));result=restore_result;count=1
    uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4+count*4);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,intercept)
rng=random.Random(87)
for case in range(900):
    kind=case%3;back=case%4!=0;width=320 if case%2 else 640
    l,t=rng.randrange(60),rng.randrange(60);length=rng.randrange(5)
    r=Rect(l,t,l+rng.randrange(5),t+rng.randrange(5))
    color=rng.getrandbits(32)
    lock_result=rng.choice([0,0,0,0x887601c2,0x8876021c,0x80004005])
    unlock_result=rng.choice([0,0x80004005]);restore_result=rng.choice([0,0x80004005])
    results=[0x8876021c]*rng.randrange(4)+[rng.choice([0,0x887601c2,0x80004005])]
    nidx=cidx=0;native.clear();actual.clear();initial=bytes([197])*65536
    u.mem_write(PIX,initial);C.memmove(pixels,initial,len(initial))
    word(0x42931c,OBJ if back else OBJ+4);word(0x42818c,width)
    u.mem_write(RECTADDR,bytes(r));sp=STACK+32768;u.reg_write(UC_X86_REG_ESP,sp)
    args=[OBJ,RECTADDR,color] if kind==0 else [OBJ,l,t,length,color]
    u.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
    u.emu_start([0x415e50,0x415c00,0x415c40][kind],STOP,count=100000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP
    expected_result=u.reg_read(UC_X86_REG_EAX)
    got=lib.sniper_surface_fill(C.byref(surface),back,width,C.byref(r),color) if kind==0 else getattr(lib,'sniper_surface_'+('vertical' if kind==1 else 'horizontal'))(C.byref(surface),back,width,l,t,length,color)
    assert native==actual,(case,native,actual)
    assert expected_result==got,(case,hex(expected_result),hex(got))
    assert bytes(pixels)==bytes(u.mem_read(PIX,65536)),case
print('900 fill/vertical/horizontal traces, return values and complete buffers match original x86')
