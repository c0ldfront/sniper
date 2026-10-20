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
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'research/experiments/critical-parity/frame'
EXE=ROOT/'code/build/assets/files/SNIPER.EXE'
image=pefile.PE(str(EXE)).get_memory_mapped_image()
BUILD=OUT/'build';BUILD.mkdir(exist_ok=True)
subprocess.run(['gcc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'code/src'),str(ROOT/'code/tests/critical_frame_bridge.c'),*[str(p) for p in sorted((ROOT/'code/src').glob('*.c'))],'-o',str(BUILD/'libcritical_frame.so')],check=True)
lib=C.CDLL(str(BUILD/'libcritical_frame.so'))
lib.frame_init.argtypes=[C.c_void_p]*3+[C.c_uint32]
lib.frame_begin.argtypes=[C.c_int32,C.c_uint32,C.c_int32]
lib.frame_weapon.argtypes=[C.c_void_p]
lib.frame_copy.argtypes=[C.c_int,C.c_void_p]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
POOL=0x100000;SHIPS=0x150000;STOP=0x810000;STACK=0x800000
allocated=pickup_allocated=clock_calls=keys=ms=0;events=[];hits={};trace=[]
scouting=False;scouted=[];chain=[];frame=-1
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def text(a):return bytes(u.mem_read(a,128)).split(b'\0')[0].decode()
def ret(sp,value=0,clean=0):
 u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_EIP,get(sp));u.reg_write(UC_X86_REG_ESP,sp+4+clean)
def hook(uc,a,size,data):
 global allocated,pickup_allocated,clock_calls
 if a in (0x413510,0x412b60,0x414b10,0x411540,0x404680,0x411620,0x4137c0,0x407a80,0x40aae0,0x4165f0,0x4130e0,0x413152,0x4020ad,0x4050c0,0x411450,0x411530,0x411560,0x411600,0x540100,0x540400):hits[a]=hits.get(a,0)+1
 sp=uc.reg_read(UC_X86_REG_ESP)
 if scouting and a==0x404f10:
  scouted.append([get(sp+4+i*4) for i in range(4)]);ret(sp);return
 if not scouting and a in (0x411560,0x415030,0x412b60,0x411600,0x411620,0x4137c0):
  chain.append({'frame':frame,'entry':hex(a),'gun':get(SHIPS+0xd0),'rng':get(0x422f78),'frequency':get(0x42def0),'team':get(SHIPS+0x68),'attached':get(SHIPS+0x6c)})
 if a==0x540100:
  value=ms+7*clock_calls;clock_calls+=1;ret(sp,value);return
 if a==0x540000:ret(sp,0x8000 if keys&{17:1,16:2,9:4,38:8,40:16,37:32,39:64}.get(get(sp+4),0) else 0,4);return
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
  if get(sp)==0x4159e4:events.append((3,C.c_int32(uc.reg_read(UC_X86_REG_ECX)).value,0,0))
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
 b=(C.c_ubyte*size)();lib.frame_copy(which,b);return bytes(b)
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
 assert struct.unpack('<4i',copied(12,16))==(get(0x429f58),get(0x4289f4),(get(0x4283e8)-SHIPS)//536,(get(0x4283ec)-SHIPS)//536),(case,frame,'team selection/order')
 return meta

# Select only initial inputs by executing the original generator. No expected-state formulas.
scouting=True
for scout_seed in range(4096):
 u.mem_write(0x400000,image);u.mem_write(0x600000,bytes(1048576))
 for a,v in [(0x429f58,0),(0x4289f4,0),(0x429f48,0x600000),(0x4353f4,0x540100),(0x520008,0),(0x422f78,scout_seed)]:put(a,v)
 ms=6010;clock_calls=0;scouted=[]
 call(0x411450,STOP,this=0x520000)
 if scouted and scouted[0][2]==8:break
else:raise AssertionError('no gun-upgrade seed in bounded native scout')
scouting=False
scout={'seed':scout_seed,'generated':scouted[0],'bounds':[0,4095]}
results=[]
for case in range(8):
 # Original CRT startup selects masked, nearest, 53-bit x87 precision.
 u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
 assert u.reg_read(UC_X86_REG_FPCW)==0x027f
 u.mem_write(0x400000,image)
 ships=(C.c_int32*268)();config=(C.c_int32*72)();lib.sniper_local_config(config);grid=(C.c_uint8*1048576)()
 for i in range(2):
  C.memmove(C.addressof(ships)+i*536,f'Pilot{i}'.encode()+b'\0',7)
  values={0x40:0,0x44:i,0x48:400000+i*80000,0x4c:400000,0x58:10000,0x5c:1000000 if i else 100000,0x68:i,0x6c:-1,0x74:100,0x80:100,0xa0:5,0xc8:1000000,0xcc:160,0xd0:2,0xd4:2,0xd8:4,0xdc:3,0xe0:10,0xe4:2000,0xe8:int(case==2),0xfc:1,0x98:int(case==2),0x100:int(case==3)}
  for off,v in values.items():ships[i*134+off//4]=v
 if case==2:ships[134+0x4c//4]=600000
 # Sparse wall supplies real wall collision and fragmentation paths.
 if case in (1,2,3):
  for y in range(20,32):grid[y*1024+32]=1
 if case==4:
  grid[25*1024+25]=152;config[(0x428295-0x4281c9)//4]=120
 if case==7:
  config[(0x4282ad-0x4281c9)//4]=1;ships[0x6c//4]=1;ships[134+0x68//4]=0
 seed=7711+case*197
 if case==6:
  seed=scout_seed
  for i in range(2):
   ships[i*134+0x48//4]=scouted[0][0]*16000-6000+i*80000
   ships[i*134+0x4c//4]=scouted[0][1]*16000+8000+i*80000
  ships[0xd0//4]=1;ships[0x5c//4]=1000000
 lib.frame_init(ships,config,grid,seed)
 u.mem_write(SHIPS,bytes(ships));u.mem_write(POOL,b'\x55'*73728);u.mem_write(0x600000,bytes(grid));u.mem_write(0x700000,bytes(8192));u.mem_write(0x180000,bytes(2560));u.mem_write(0x429288,bytes(64));u.mem_write(0x4281c9,bytes(config))
 for a,n in [(0x429748,2048),(0x424330,16),(0x4287e8,512),(0x428a50,16)]:u.mem_write(a,bytes(n))
 for a,v in [(0x429f58,0),(0x4289f4,0),(0x429f48,0x600000),(0x429f50,0x700000),(0x42de7c,32),(0x429318,0),(0x429328,2),(0x4218e0,SHIPS),(0x424348,0x520000),(0x520000,0x420008),(0x520008,100),(0x42190c,143),(0x4218e8,148),(0x4218f8,177),(0x4218ec,121),(0x4218f4,124),(0x4218f0,123),(0x421908,134),(0x428190,162),(0x428194,164),(0x428198,165),(0x428168,166),(0x42816c,167),(0x428170,168),(0x42818c,640),(0x4353f4,0x540100),(0x4354b8,0x540000),(0x4354c0,0x540000),(0x4354d0,0x540400),(0x4218fc,125),(0x421900,127),(0x421904,169),(0x42e144,1),(0x422234,1),(0x428160,0),(0x42def0,0),(0x42e198,100),(0x4218cc,0),(0x42833c,0),(0x4281c0,0),(0x428a68,0),(0x428a00,0),(0x428a70,0),(0x4289f0,0),(0x422f78,seed),(0x428178,123),(0x429738,0),(0x428a08,0),(0x42930c,0),(0x429300,0),(0x4283d8,0),(0x4292fc,0),(0x4218d0,0),(0x424340,-1),(0x422278,0),(0x4292e8,0),(0x4292f0,-1),(0x42229c,0),(0x4222a0,0),(0x42434c,480)]:put(a,v)
 for i in range(2):put(0x429338+i*4,SHIPS+i*536);put(0x4283e8+i*4,SHIPS+i*536)
 if case==7:put(0x42def0,10003);lib.frame_initial_frequency(10003)
 allocated=pickup_allocated=clock_calls=0;ms=1000;hits={};dead_frames=respawn_frames=0;last_dead=False;max_active=0;timeline=[];transitions=[];chain=[]
 for frame in range(180):
  dt=[0,1,2,5,11,25,50][(frame+case)%7];ms+=max(dt*10+case,7*clock_calls)
  keys=([1,1,0,4,0,3,0][frame%7] if frame<18 else (1 if frame%19==0 else 0))
  if case!=4:keys|=([8,8,64,0,32,16,0][(frame//7)%7])
  if case==6:keys=0 if frame<60 else 1
  if case==7 and frame==0:keys=0
  events=[];clock_calls=0;put(0x4218c8,dt)
  lib.frame_begin(dt,ms,keys)
  # Replay a peer weapon event through actual409d70, not direct damage injection.
  if frame in (3,35,90) and case not in (4,6):
   x=get(SHIPS+0x48);y=get(SHIPS+0x4c)
   weapon=(C.c_int32*14)([1,3,2,4,1,3,1,2][case],2,1,1,4,0,0,10000,x-48000,y,0,0,1,1)
   call(0x409d70,STOP,weapon);lib.frame_weapon(weapon)
  if case==5 and frame==1:
   attach=(C.c_int32*1)(1);call(0x411620,STOP,attach,0x520000);lib.frame_attach(1)
  call(0x4020ad,0x402468);lib.frame_step()
  meta=compare(case,frame);max_active=max(max_active,meta[1]);dead=get(SHIPS+0x5c)<0
  dead_frames+=dead
  if last_dead and not dead:respawn_frames+=1
  if last_dead != dead:transitions.append({'frame':frame,'event':'death' if dead else 'respawn','rng':meta[0],'x':get(SHIPS+0x48),'y':get(SHIPS+0x4c)})
  last_dead=dead
  if frame%15==0 or (dead and not timeline):timeline.append({'frame':frame,'ticks':dt,'energy':get(SHIPS+0x5c),'death_timer':get(SHIPS+0x60),'respawn_timer':get(SHIPS+0x64),'projectiles':meta[1],'rng':meta[0],'clock_calls':meta[10]})
 results.append({'scenario':case,'frames':180,'allocated':allocated,'maximum_active':max_active,'dead_frames':dead_frames,'respawns':respawn_frames,'native_calls':{hex(a):n for a,n in hits.items()},'timeline':timeline,'transitions':transitions,'pickup_allocated':pickup_allocated,'contact_award_weapon_trace':chain})
 print('PASS scenario',case,'allocations',allocated,'dead frames',dead_frames,'respawns',respawn_frames,flush=True)
result={'native_setup_scout':scout,'numeric_environment':{'native_x87_control_word':'0x027f','host':'baseline037f then sniper_numeric_initialize; asserted027f'},'original_sha256':hashlib.sha256(EXE.read_bytes()).hexdigest(),'scope':'Actual original4020ad..402468 active gameplay frames versus production sniper_session_step: clock, timed pickup generation, complete idle-command input prefix, asynchronous flight/weapons, position, safe deadline, simulation, private positional audio and camera RNG; OS/device/event-loop/network transport seams remain','scenarios':results}
(OUT/'result.json').write_text(json.dumps(result,indent=2)+'\n')
assert all(r['native_calls'].get('0x4020ad',0)==180 and r['native_calls'].get('0x4050c0',0)==180 and r['native_calls'].get('0x411450',0)==180 for r in results), 'whole frame/input/backend execution'
assert all(r['pickup_allocated']>0 for r in results), 'actual timed generation'
assert results[4]['native_calls'].get('0x540400',0)>0, 'safe deadline posts request'
assert all(r['native_calls'].get('0x411530',0)>0 for r in results), 'actual position dispatch'
assert any(r['respawns']>0 for r in results), 'actual chained respawn'
assert any(e['entry']=='0x411560' for e in results[6]['contact_award_weapon_trace']), 'generated pickup contact'
assert any(e['entry']=='0x415030' for e in results[6]['contact_award_weapon_trace']), 'generated pickup award'
assert any(e['entry']=='0x412b60' and e['frame']>=60 and e['gun']==2 for e in results[6]['contact_award_weapon_trace']), 'later upgraded gun fire'
assert results[7]['native_calls'].get('0x411600',0)==1, 'automatic team rollover'
assert any(e['entry']=='0x4137c0' and e['frame']==0 for e in results[7]['contact_award_weapon_trace']), 'rollover reset'
assert any(e['entry']=='0x411620' and e['frame']==0 for e in results[7]['contact_award_weapon_trace']), 'rollover detach'
print('PASS1440 complete active gameplay frames: clock, flight/input/position, generation, safe deadline, combat, audio/camera RNG')
