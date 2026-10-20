"""Native40a840 activity setter/40a8c0 cache-stop traces; COM Stop intercepted."""
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
events=[]
def hook(uc,a,size,user):
 if a!=0x530000:return
 sp=uc.reg_read(UC_X86_REG_ESP);events.append(get(sp+4));uc.reg_write(UC_X86_REG_EAX,0);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(1200):
 available=case%5!=0;active=r.choice([0,0,1,2,-1]);old=r.choice([-1,0,1]);count=r.randrange(18);expected=[]
 for a,x in [(0x42debc,available),(0x42deb0,old),(0x42dec4,count),(0x42dec8,0x500000),(0x4281c4,0xdeadbeef),(0x520048,0x530000),(0x800000,0x540000),(0x800004,active)]:put(a,x)
 for i in range(count):
  obj=0x501000+i*0x100;put(0x500000+4*i,obj);voices=r.randrange(17);put(obj+0x44,voices)
  for j in range(voices):
   buffer=0x510000+i*0x100+j*4;put(buffer,0x520000);put(obj+j*4,buffer);expected.append(buffer)
 for entry in [0x40a840,0x40a8c0]:
  put(0x42deb0,old);events.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(entry,0x540000,count=10000)
  want=expected if available and (entry==0x40a8c0 or not active) else []
  assert events==want,(case,entry,events,want)
  assert get(0x42deb0)==(active if available and entry==0x40a840 else old)
print('PASS2400 native activity/cache-stop traces:availability,active state,cache/voice order,repeated deactivation,dynamic voice excluded')
