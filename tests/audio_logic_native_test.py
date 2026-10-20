"""Original positional stage and dispatcher, with DirectSound COM seams."""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-O2','-shared','-fPIC',str(root/'code/src/audio_logic.c'),'-o',str(root/'code/build/libaudio_logic.so')],check=True)
lib=C.CDLL(str(root/'code/build/libaudio_logic.so'))
class Request(C.Structure):_fields_=[(s,C.c_int32) for s in ['volume','pan','loop','pitch']]
lib.sniper_audio_position.argtypes=[C.c_int32]*6+[C.POINTER(Request)]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
events=[];mode=0;status=0;status_ok=0;random_value=0
methods={0x530000+i*16:i for i in [9,15,16,13,17,12,20]}
def hook(uc,a,size,user):
 sp=uc.reg_read(UC_X86_REG_ESP);clean=0
 if mode==0 and a==0x4165f0:events.append(('rng',));uc.reg_write(UC_X86_REG_EAX,random_value)
 elif mode==0 and a==0x40aae0:events.append(tuple(get(sp+k) for k in [4,8,12,16]));clean=16
 elif mode==1 and a==0x40abb0:events.append(('fill',))
 elif mode==1 and a in methods:
  m=methods[a];clean={9:8,15:8,16:8,13:8,17:8,12:16,20:4}[m]
  if m==9:put(get(sp+8),status);uc.reg_write(UC_X86_REG_EAX,0 if status_ok else 1)
  else:events.append((m,)+tuple(get(sp+k) for k in range(8,clean+1,4)));uc.reg_write(UC_X86_REG_EAX,0)
 else:return
 uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(2400):
 mode=0;events.clear();lx=r.randrange(-1000000,17000000);ly=r.randrange(-1000000,17000000)
 x=lx//1000+r.randrange(-1000,1001);y=ly//1000+r.randrange(-1000,1001);present=case%13!=0;width=r.choice([320,640,800,1024,-640]);spread=r.randrange(1,1001);random_value=r.randrange(32768)
 for a,v in [(0x4218e0,0x500000 if present else 0),(0x500048,lx),(0x50004c,ly),(0x42818c,width),(0x4282a5,spread),(0x800000,0x540000),(0x800004,123),(0x800008,x),(0x80000c,y)]:put(a,v)
 u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x407a80,0x540000,count=5000)
 req=Request();accepted=lib.sniper_audio_position(x,y,lx,ly,present,width,C.byref(req))
 expected=[('rng',),(req.volume,req.pan,0,lib.sniper_audio_pitch(random_value,spread))] if accepted else []
 assert events==expected,(case,events,expected)
for case in range(2400):
 mode=1;events.clear();available=case%7!=0;enabled=case%5!=0;count=case%17;status=case%8;status_ok=case%11!=0;index=r.randrange(count) if count else 0;base=r.choice([11025,22050,44100,2147483647]);pitch=r.randrange(-20000,20001);volume=r.randrange(-10000,1);pan=r.randrange(-20000,20001);loop=r.choice([0,1,-1,2]);resource=r.choice([123,-1])
 for i in range(16):put(0x500000+i*4,0x510000)
 for a,v in [(0x510000,0x520000),(0x42debc,available),(0x42deb0,enabled),(0x500040,resource),(0x500044,count),(0x500048,index),(0x50004c,base),(0x800000,0x540000),(0x800004,volume),(0x800008,pan),(0x80000c,loop),(0x800010,pitch)]:put(a,v)
 for a,m in methods.items():put(0x520000+m*4,a)
 u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x40aae0,0x540000,count=5000)
 active=lib.sniper_audio_should_play(available,enabled,count,status_ok,status);expected=[]
 if active:
  if status&2 and resource!=-1:expected=[(20,),('fill',)]
  expected += [(15,volume),(16,pan),(13,0),(17,lib.sniper_audio_frequency(base,pitch)),(12,0,0,int(loop!=0))]
 assert events==expected,(case,events,expected)
 assert get(0x500048)==((index+1)%count if active else index)
print('PASS4800 native audio cases: positional distance/pan/RNG gates, COM call order, looping suppression, restoration, frequency wrapping and voice rotation')
