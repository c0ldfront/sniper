"""Persistent original active gameplay frame versus real sniper_session_step.
All original clock/backend/input/flight/position/simulation/camera code executes.
External weapons/attachment and OS/device/allocator services are declared seams.
"""
from pathlib import Path
import ctypes as C
import hashlib
import json
import struct
import subprocess
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_EDI, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'research/experiments/critical-parity/capabilities'
EXE=ROOT/'code/build/assets/files/SNIPER.EXE'
image=pefile.PE(str(EXE)).get_memory_mapped_image()
BUILD=OUT/'build';BUILD.mkdir(parents=True,exist_ok=True)
subprocess.run(['gcc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'code/src'),str(ROOT/'code/tests/critical_capabilities_bridge.c'),*[str(p) for p in sorted((ROOT/'code/src').glob('*.c'))],'-Wl,--wrap=sniper_position_dispatch','-Wl,--wrap=sniper_weapon_action_fields','-o',str(BUILD/'libcritical_capabilities.so')],check=True)
lib=C.CDLL(str(BUILD/'libcritical_capabilities.so'))
lib.cap_init.argtypes=[C.c_void_p]*3+[C.c_uint32]
lib.cap_begin.argtypes=[C.c_int32,C.c_uint32,C.c_int32]
lib.cap_weapon.argtypes=[C.c_void_p]
lib.cap_copy.argtypes=[C.c_int,C.c_void_p]
lib.cap_mouse.argtypes=[C.c_int32,C.c_int32]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
POOL=0x100000;SHIPS=0x150000;STOP=0x810000;STACK=0x800000
allocated=pickup_allocated=clock_calls=keys=ms=0;events=[];hits={};trace=[];native_payloads=[]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def text(a):return bytes(u.mem_read(a,128)).split(b'\0')[0].decode()
def ret(sp,value=0,clean=0):
 u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_EIP,get(sp));u.reg_write(UC_X86_REG_ESP,sp+4+clean)
def hook(uc,a,size,data):
 global allocated,pickup_allocated,clock_calls
 if a in (0x413510,0x412b60,0x414b10,0x411540,0x404680,0x411620,0x4137c0,0x407a80,0x40aae0,0x4165f0,0x4130e0,0x413152,0x4020ad,0x4050c0,0x411450,0x411530,0x411560,0x411600,0x540100,0x540400):hits[a]=hits.get(a,0)+1
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a==0x406431:events.append((15,uc.reg_read(UC_X86_REG_EBP),uc.reg_read(UC_X86_REG_EDI),0))
 if a==0x406922:events.append((16,sum((1<<i) for i in range(5) if get(SHIPS+0x88+4*i)),get(SHIPS+0x5c),get(0x4292fc)))
 if a==0x411530:
  args=[get(sp+4+i*4) for i in range(13)];native_payloads.append(list(args))
  if not args[5]:args[6:11]=[0]*5
  events.extend([(10,*args[:3]),(11,*args[3:6]),(12,*args[6:9]),(13,*args[9:12]),(14,args[12],0,0)])
 if a==0x540100:
  value=ms+7*clock_calls;clock_calls+=1;ret(sp,value);return
 if a==0x540000:ret(sp,0x8000 if keys&{1:128,17:1,16:2,9:4,38:8,40:16,37:32,39:64}.get(get(sp+4),0) else 0,4);return
 if a==0x416b20:
  if get(sp+4)==20:
   events.append((6,pickup_allocated,0,0));value=0x180000+pickup_allocated*20;pickup_allocated+=1;ret(sp,value);return
  assert get(sp+4)==72
  assert allocated<1024
  events.append((1,allocated,0,0));value=POOL+allocated*72;allocated+=1;ret(sp,value);return
 if a==0x416620:
  address=get(sp+4);events.append((7,(address-0x180000)//20,0,0) if 0x180000<=address<0x181000 else (2,(address-POOL)//72,0,0));ret(sp);return
 if a==0x407a80:events.append((3,get(sp+4),get(sp+8),get(sp+12)));return
 if a==0x40aab0:events.append((9,C.c_int32(uc.reg_read(UC_X86_REG_ECX)).value,0,0));ret(sp);return
 if a==0x540400:events.append((8,0,0,0));ret(sp,0,16);return
 if a==0x40aae0:
  events.append((4,C.c_int32(uc.reg_read(UC_X86_REG_ECX)).value,get(sp+4),get(sp+8)));events.append((5,get(sp+12),get(sp+16),0));ret(sp,0,16);return
 if a==0x416bc0:
  fmt=text(get(sp+8));args=[text(get(sp+12)),get(sp+16)]
  if fmt.count('%s')==2:args.append(text(get(sp+20)))
  value=(fmt%tuple(args)).encode();uc.mem_write(get(sp+4),value+b'\0');ret(sp,len(value));return
u.hook_add(UC_HOOK_CODE,hook)

def call(start,end,args=None,this=None):
 u.reg_write(UC_X86_REG_ESP,STACK);put(STACK,STOP)
 if args is not None:u.mem_write(STACK+4,bytes(args))
 if this is not None:u.reg_write(UC_X86_REG_ECX,this)
 u.emu_start(start,end,count=3000000)
 assert u.reg_read(UC_X86_REG_EIP)==end,hex(u.reg_read(UC_X86_REG_EIP))

def copied(which,size):
 b=(C.c_ubyte*size)();lib.cap_copy(which,b);return bytes(b)
def compare(case,frame):
 meta=struct.unpack('<11i',copied(4,44))
 expected=[get(a) for a in (0x422f78,0x4289f0,0x428178,0x429738)]+[allocated,len(events)]+[get(a) for a in (0x429300,0x4283d8,0x4292fc,0x42930c)]+[clock_calls]
 assert list(meta)==expected,(case,frame,'globals',meta,expected)
 for which,address,size,name in [(0,SHIPS,1072,'ships'),(1,POOL,73728,'projectile pool'),(2,0x700000,8192,'coverage'),(7,0x600000,1048576,'map')]:
  c=copied(which,size);n=bytes(u.mem_read(address,size))
  if c!=n:
   differences=[(hex(i),c[i],n[i]) for i in range(size) if c[i]!=n[i]]
   raise AssertionError((case,frame,name,differences[:20]))
 hud=copied(3,544);native=b''.join(bytes(u.mem_read(a,n)) for a,n in [(0x424330,16),(0x4287e8,512),(0x428a50,16)])
 assert hud==native,(case,frame,'notices')
 indices=struct.unpack('<512i',copied(5,2048))
 expected=[(get(0x429748+i*4)-POOL)//72 if get(0x429748+i*4) else -1 for i in range(512)]
 assert list(indices)==expected,(case,frame,'projectile list')
 ce=list(struct.iter_unpack('<4i',copied(6,len(events)*16)))
 assert ce==events,(case,frame,'ordered effects',ce,events)
 assert struct.unpack('<2i',copied(8,8)) == (get(0x42de80),get(0x424324)),(case,frame,'camera')
 state=struct.unpack('<12i',copied(9,48));expected=[get(a) for a in (0x42e198,0x4218c8,0x4218cc,0x42def0,0x4281c0,0x428a68,0x428a00,0x428a70,0x42833c,0x428a08,0x520008)]+[pickup_allocated]
 assert list(state)==expected,(case,frame,'frame/position',state,expected)
 assert copied(10,2560)==bytes(u.mem_read(0x180000,2560)),(case,frame,'pickup objects')
 indices=struct.unpack('<16i',copied(11,64));expected=[(get(0x429288+4*i)-0x180000)//20 if get(0x429288+4*i) else -1 for i in range(16)]
 assert list(indices)==expected,(case,frame,'pickup list')
 return meta

results=[]
for case in range(3):
 u.mem_write(0x400000,image)
 u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
 ships=(C.c_int32*268)();config=(C.c_int32*72)();lib.sniper_local_config(config);grid=(C.c_uint8*1048576)()
 for i in range(2):
  C.memmove(C.addressof(ships)+i*536,f'Pilot{i}'.encode()+b'\0',7)
  values={0x40:0,0x44:i,0x48:400000+i*80000,0x4c:400000,0x58:10000,0x5c:1000000 if i else 100000,0x68:i,0x6c:-1,0x74:100,0x80:100,0xa0:5,0xc8:1000000,0xcc:160,0xd0:2,0xd4:2,0xd8:4,0xdc:3,0xe0:10,0xe4:2000,0xe8:int(case==2),0xfc:1,0x98:int(case==2),0x100:int(case==3)}
  for off,v in values.items():ships[i*134+off//4]=v
 # Native capabilities are fixture inventory; actual enabled bits start off.
 for off in [0xec,0xf0,0xf4,0xf8,0xfc]:ships[off//4]=int(case!=2)
 for off in [0x88,0x8c,0x90,0x94,0x98]:ships[off//4]=0
 ships[0x40//4]=2;ships[0x5c//4]=1000000 if case!=1 else 20000
 ships[0xd8//4]=0;ships[0xc4//4]=0
 ships[134+0x48//4]=8000000;ships[134+0x4c//4]=8000000
 seed=7711+case*197
 lib.cap_init(ships,config,grid,seed)
 u.mem_write(SHIPS,bytes(ships));u.mem_write(POOL,b'\x55'*73728);u.mem_write(0x600000,bytes(grid));u.mem_write(0x700000,bytes(8192));u.mem_write(0x180000,bytes(2560));u.mem_write(0x429288,bytes(64));u.mem_write(0x4281c9,bytes(config))
 for a,n in [(0x429748,2048),(0x424330,16),(0x4287e8,512),(0x428a50,16)]:u.mem_write(a,bytes(n))
 for a,v in [(0x429f48,0x600000),(0x429f50,0x700000),(0x42de7c,32),(0x429318,0),(0x429328,2),(0x4218e0,SHIPS),(0x424348,0x520000),(0x520000,0x420008),(0x520008,100),(0x4218f8,177),(0x4218ec,121),(0x4218f4,124),(0x4218f0,123),(0x421908,134),(0x428190,162),(0x428194,164),(0x428198,165),(0x428168,166),(0x42816c,167),(0x428170,168),(0x42818c,640),(0x4353f4,0x540100),(0x4354b8,0x540000),(0x4354c0,0x540000),(0x4354d0,0x540400),(0x4218fc,125),(0x421900,127),(0x421904,169),(0x42e144,1),(0x422234,1),(0x428160,0),(0x42def0,0),(0x42e198,100),(0x4218cc,0),(0x42833c,0),(0x4281c0,0),(0x428a68,0),(0x428a00,0),(0x428a70,0),(0x4289f0,0),(0x422f78,seed),(0x428178,123),(0x429738,0),(0x428a08,0),(0x42930c,0),(0x429300,0),(0x4283d8,0),(0x4292fc,0),(0x4218d0,0),(0x424340,-1),(0x422278,0),(0x4292e8,0),(0x4292f0,-1),(0x42229c,0),(0x4222a0,0),(0x42434c,480)]:put(a,v)
 for i in range(2):put(0x429338+i*4,SHIPS+i*536);put(0x4283e8+i*4,SHIPS+i*536)
 allocated=pickup_allocated=clock_calls=0;ms=1000;hits={};timeline=[];actions=set();flags=set();toggle_on=set();toggle_off=set();last=[0]*5;last_energy=get(SHIPS+0x5c);energy_shutoffs=[];cloak_shots=[]
 for frame in range(120):
  dt=10;ms+=max(100,7*clock_calls)
  # Two actual clicks per capability, with flight and delayed weapon input.
  action=2+frame//12 if frame<60 else 2+(frame-60)//3 if frame<75 else None
  phase=frame%12 if frame<60 else (frame-60)%3
  click=action is not None and (phase in [0,6] if frame<60 else phase==0)
  keys=(128 if click else 0)|(8 if frame%5<2 else 64 if frame%5==2 else 0)
  if frame<60 and phase==3 and action in (3,6):keys|=1
  x,y=(20,240+int(action)*8+1) if click else (200,200)
  events=[];native_payloads=[];clock_calls=0;put(0x4218c8,dt);put(0x42229c,x);put(0x4222a0,y)
  lib.cap_begin(dt,ms,keys);lib.cap_mouse(x,y)
  call(0x4020ad,0x402468);lib.cap_step()
  meta=compare(case,frame)
  enabled=[get(SHIPS+off) for off in [0x88,0x8c,0x90,0x94,0x98]]
  for i,(old,new) in enumerate(zip(last,enabled)):
   if not old and new:toggle_on.add(i+2)
   if old and not new:toggle_off.add(i+2)
  after_action=next(e for e in events if e[0]==16)
  for i in range(4):
   if after_action[1]&(1<<i) and not enabled[i]:energy_shutoffs.append((frame,i+2))
  if last[1] and keys&1 and not (after_action[1]&2):cloak_shots.append(frame)
  for e in events:
   if e[0]==15 and e[1]:actions.add(e[2])
   if e[0]==13:flags.add(e[3])
  timeline.append({'frame':frame,'mouse_action':action if click else None,'keys':keys,
   'enabled':enabled,'energy':get(SHIPS+0x5c),'energy_delta':get(SHIPS+0x5c)-last_energy,
   'position':[get(SHIPS+0x48),get(SHIPS+0x4c)],'velocity':[get(SHIPS+0x50),get(SHIPS+0x54)],
   'native_position_payloads':native_payloads,'cooldown':meta[6],'rng':meta[0],'projectiles':meta[1],'events':list(events)})
  last=enabled;last_energy=get(SHIPS+0x5c)
 assert set(range(2,7))<=actions,(case,'missing input actions',actions)
 assert hits.get(0x4020ad)==120 and hits.get(0x4050c0)==120 and hits.get(0x411530,0)>0
 if case==0:assert toggle_on==set(range(2,7)) and toggle_off==set(range(2,7)),(toggle_on,toggle_off)
 if case==2:assert not toggle_on,('unavailable enabled',toggle_on)
 results.append({'scenario':case,'frames':120,'actions':sorted(actions),'enabled_actions':sorted(toggle_on),
  'energy_shutoffs':energy_shutoffs,'cloak_shots':cloak_shots,'disabled_actions':sorted(toggle_off),'position_flags':sorted(flags),'native_calls':{hex(k):v for k,v in hits.items()},'timeline':timeline})
 print('PASS capability scenario',case,'enabled',sorted(toggle_on),'disabled',sorted(toggle_off),flush=True)
assert results[1]['energy_shutoffs'],'energy-driven shutoff scenario did not disable'
assert results[0]['cloak_shots'],'firing did not disable cloak'
result={'original_sha256':hashlib.sha256(EXE.read_bytes()).hexdigest(),'frames':360,'scope':'Persistent full offline session frames with actual mouse action2..6 selection, subsequent energy/flight/position/weapon effects; declared OS/device seams','scenarios':results}
(OUT/'result.json').write_text(json.dumps(result,indent=2)+'\n')
paths=[EXE,Path(__file__),ROOT/'code/tests/critical_capabilities_bridge.c',*sorted((ROOT/'code/src').glob('*.c')),*sorted((ROOT/'code/src').glob('*.h')),BUILD/'libcritical_capabilities.so']
(OUT/'manifest.json').write_text(json.dumps({str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},indent=2)+'\n')
print('PASS360 persistent capability frames: complete native frame/input, mouse toggle actions2..6, energy, flight, position payloads, actual projectiles and shared audio RNG')
