"""409d70, allocation/constructor/update seams capture original dispatch."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
Create=C.CFUNCTYPE(C.c_void_p,C.c_void_p,C.POINTER(C.c_int32));Step=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_void_p,C.c_int32);Release=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p)
class World(C.Structure):_fields_=[('items',C.POINTER(C.c_void_p)),('count',C.POINTER(C.c_int32)),('group',C.POINTER(C.c_int32)),('speed',C.c_int32),('spread',C.c_int32),('create',Create),('update',Step),('release',Release),('user',C.c_void_p)]
items=(C.c_void_p*512)();count=C.c_int32();group=C.c_int32();ce=[];ne=[];ca=0;na=0;fail=-1;remove_mask=0
@Create
def create(_,args):
    global ca
    index=ca;ca+=1;ce.append(('allocate',72));p=None if index==fail else 0x530000+index*72
    if p:ce.append(('construct',p,tuple(args[i] for i in range(15))))
    return p
@Step
def update(_,p,dt):
    ce.append(('update',p or 0,dt));return bool(remove_mask&(1<<((p-0x530000)//72 if p else 0)))
@Release
def release(_,p):ce.append(('release',p or 0))
w=World(items,C.pointer(count),C.pointer(group),2000,500,create,update,release,None)
lib=C.CDLL(str(ROOT/'code/build/libweapon_create.so'));lib.sniper_weapon_create.argtypes=[C.POINTER(World),C.POINTER(C.c_int32)]
def hook(uc,a,size,data):
    global na
    if a not in [0x416b20,0x412b60,0x413010,0x416620]:return
    sp=uc.reg_read(UC_X86_REG_ESP);p=uc.reg_read(UC_X86_REG_ECX);value=0;clean=0
    if a==0x416b20:
        ne.append(('allocate',get(sp+4)));value=0 if na==fail else 0x530000+na*72;na+=1
    elif a==0x412b60:ne.append(('construct',p,tuple(get(sp+4+i*4) for i in range(15))));value=p;clean=60
    elif a==0x413010:ne.append(('update',p,get(sp+4)));value=bool(remove_mask&(1<<((p-0x530000)//72 if p else 0)));clean=4
    else:ne.append(('release',get(sp+4)))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x409d70);constructs=0
for case in range(1200):
    count.value=rng.choice([0,1,508,509,510,511]);group.value=rng.choice([123,2147483647]);w.spread=rng.choice([0,500,10000])
    for i in range(512):items[i]=0x550000+i*72;put(0x429748+4*i,items[i])
    args=(C.c_int32*14)(case%6,case%3,1,2,5,(case//6)%2,rng.choice([0,1,10]),rng.choice([0,100,39999,-1,-40001,2147483647]),400000,500000,123,-234,1,2)
    fail=case%4-1;remove_mask=case%8;ca=na=0;ce.clear();ne.clear()
    for a,v in [(0x4289f0,count.value),(0x428178,group.value),(0x4281ed,w.speed),(0x428209,w.spread),(0x800000,0x810000)]:put(a,v)
    u.mem_write(0x800004,bytes(args));u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x409d70,0x810000,count=100000)
    lib.sniper_weapon_create(C.byref(w),args)
    assert ce==ne,(case,ce,ne)
    assert (count.value,group.value)==(get(0x4289f0),get(0x428178)),case
    for i in range(512):assert (items[i] or 0)==get(0x429748+4*i),(case,i)
    constructs+=sum(e[0]=='construct' for e in ce)
assert constructs>0
print(f'1200 native weapon events match constructor args,update/free order,list and group IDs;{constructs} constructions')
