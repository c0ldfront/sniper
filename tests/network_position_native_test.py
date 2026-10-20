"""Full original404a60 with real effect415b80; controlled weapon/movement calls."""
import ctypes as C,pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p);Weapon=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int32));Predict=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Ship),C.c_int32,*([C.POINTER(C.c_int32)]*4))
class Position(C.Structure):_fields_=[('players',C.POINTER(C.POINTER(Ship))),('movement',C.c_void_p),('clock',Clock),('weapon',Weapon),('predict',Predict),('user',C.c_void_p)]
lib=C.CDLL(str(root/'code/build/libnetwork_backend.so'));lib.sniper_network_position.argtypes=[C.POINTER(Position),C.POINTER(C.c_int32)]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def signed(x):return C.c_int32(x).value
def ret(clean=0):
 sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,get(sp));u.reg_write(UC_X86_REG_ESP,sp+4+clean)
ce=[];ne=[];ms=0;replacement=[];mutate=False
@Clock
def clock(_):ce.append(('clock',));return ms
@Weapon
def weapon(_,a):
 ce.append(('weapon',tuple(a[:14]),bytes(ship)))
 if mutate:ship.word[0x48//4]=12345;ship.word[0x4c//4]=-54321
@Predict
def predict(_,s,ticks,x,y,vx,vy):
 ce.append(('move',ticks,tuple(p[0] for p in (x,y,vx,vy)),bytes(s.contents)))
 for p,v in zip((x,y,vx,vy),replacement):p[0]=v
 if mutate:s.contents.word[0x48//4]=-20000

def hook(uc,a,size,user):
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a==0x600000:ne.append(('clock',));uc.reg_write(UC_X86_REG_EAX,ms);ret()
 elif a==0x409d70:
  ne.append(('weapon',tuple(signed(get(sp+4+4*i)) for i in range(14)),bytes(uc.mem_read(0x500000,536))))
  if mutate:put(0x500048,12345);put(0x50004c,-54321)
  ret()
 elif a==0x414450:
  assert uc.reg_read(UC_X86_REG_ECX)==0x500000 and get(sp+24)==0
  ptrs=[get(sp+8+4*i) for i in range(4)]
  ne.append(('move',signed(get(sp+4)),tuple(signed(get(p)) for p in ptrs),bytes(uc.mem_read(0x500000,536))))
  for p,v in zip(ptrs,replacement):put(p,v)
  if mutate:put(0x500048,-20000)
  ret(24)
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x600000);r=random.Random(8760)
for case in range(2400):
 ship=Ship((C.c_int32*134)(*[r.randrange(-2**31,2**31) for _ in range(134)]));ship.word[0x214//4]=r.randrange(12)
 ship.word[0x48//4]=r.randrange(-200000,200000);ship.word[0x4c//4]=r.randrange(-200000,200000)
 a=[r.randrange(-2**31,2**31) for _ in range(15)];a[1]=3;a[7]=r.choice([0,1,2,3,4,5]);a[14]=r.choice([0,-1,1,100,2**31-1,-2**31]);ms=r.randrange(2**32);mutate=case%4==0
 replacement=[signed(ship.word[0x48//4]+r.choice([0,4000,4001,64000,64001,-64001,0x80000000])),signed(ship.word[0x4c//4]+r.choice([0,-4000,-4001,-64000,-64001,64001,0x80000000])),r.randrange(-2**31,2**31),r.randrange(-2**31,2**31)]
 missing=case%17==0;players=(C.POINTER(Ship)*1024)();players[3]=None if missing else C.pointer(ship);put(0x429338+12,0 if missing else 0x500000)
 u.mem_write(0x500000,bytes(ship));put(0x800000,0x700000)
 for i,v in enumerate(a):put(0x800004+i*4,v)
 ce.clear();ne.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x404a60,0x700000,count=10000)
 context=Position(players,None,clock,weapon,predict,None);lib.sniper_network_position(C.byref(context),(C.c_int32*15)(*a))
 assert ce==ne,(case,ce,ne)
 assert bytes(ship)==bytes(u.mem_read(0x500000,536)),case
print('PASS2400 full original remote-position updates: ship536 bytes, actual effect ring, weapon args, prediction order, signed smoothing/overflow and callback mutations')
