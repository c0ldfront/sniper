"""Stateful wire deaths: actual decoder/death/damage/pickup/audio and detach send.
Only OS I/O/time and the outgoing weapon-construction boundary are intercepted.
"""
import ctypes as C,hashlib,json,pathlib,random,struct,subprocess,sys
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EAX,UC_X86_REG_FPCW
ROOT=pathlib.Path(__file__).resolve().parents[2];OUT=ROOT/'research/experiments/critical-parity/network';OUT.mkdir(parents=True,exist_ok=True)
TEAM=False
PROJECTILES=True
BUILD=OUT/'build';BUILD.mkdir(exist_ok=True)
BRIDGE=BUILD/'bridge_join.so'
if TEAM:OUT=OUT/'team';OUT.mkdir(exist_ok=True)
OUT=OUT/'join';OUT.mkdir(exist_ok=True)
sources=sorted((ROOT/'code/src').glob('*.c'))+[ROOT/'code/tests/critical_network_join_bridge.c']
subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(ROOT/'code/src'),*map(str,sources),'-lm','-o',str(BRIDGE)],check=True)
lib=C.CDLL(str(BRIDGE));assert lib.critical_network_numeric()==0x027f
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p);Read=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_void_p,C.c_int32,C.c_int);Send=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_void_p,C.c_int32);Weapon=C.CFUNCTYPE(None,C.POINTER(C.c_int32),C.c_void_p)
lib.critical_join_init.argtypes=[Clock,Read,Send,Weapon,C.c_void_p,C.c_void_p,C.c_uint32,C.c_int];lib.critical_network_ships.restype=C.c_void_p;lib.critical_network_config.restype=C.c_void_p;lib.critical_network_team_state.argtypes=[C.POINTER(C.c_int32)];lib.critical_network_state.argtypes=[C.POINTER(C.c_uint32)]
exe=ROOT/'code/build/assets/files/SNIPER.EXE';u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000);u.mem_write(0x400000,pefile.PE(str(exe)).get_memory_mapped_image())
NET=0x500000;GAME=0x550000;SHIP=0x560000;MAP=0x600000;CLOCK=0x750000;RETURN=0x760000;STACK=0x800000
stream=[];cursor=0;calls=0;base_ms=100000;events=[];native_entries={};allocated=0;death_allocated=0;heap=0x200000;join_allocated=0

def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def ret(clean=0):
 sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,get(sp));u.reg_write(UC_X86_REG_ESP,sp+4+clean)
@Clock
def clock(_):
 global calls
 value=(base_ms+calls*13)&0xffffffff;events.append(('clock',value));calls+=1;return value
@Read
def read(_,ptr,n,peek):
 global cursor
 if cursor==len(stream):events.append(('read',n,peek,-1));return -1
 data=stream[cursor][:n];C.memmove(ptr,data,len(data));events.append(('read',n,peek,len(data)))
 if not peek:cursor+=1
 return len(data)
@Send
def send(_,ptr,n):events.append(('send',C.string_at(ptr,n).hex()));return n
@Weapon
def weapon(a,ship):events.append(('weapon',tuple(a[:14]),C.string_at(ship,536).hex()) if ship else ('effect',tuple(a[:4])))
def hook(uc,a,size,user):
 global cursor,allocated,death_allocated,heap,join_allocated
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a in (0x411d00,0x401920,0x404a60,0x414450,0x415b80,0x404530,0x4137c0,0x750100,0x409d70,0x412b60,0x413010,0x40aae0,0x404680,0x414b10,0x411cc0,0x401610,0x404340,0x404600,0x4136d0,0x404530):native_entries[hex(a)]=native_entries.get(hex(a),0)+1
 if a==CLOCK:uc.reg_write(UC_X86_REG_EAX,clock(None));ret();return
 if a==0x41614c:
  n=get(sp+12);events.append(('send',bytes(uc.mem_read(get(sp+8),n)).hex()));uc.reg_write(UC_X86_REG_EAX,n);ret(16);return
 if a==0x416152:
  n=get(sp+12);peek=int(get(sp+16)==2)
  if cursor==len(stream):length=-1
  else:
   data=stream[cursor][:n];length=len(data);uc.mem_write(get(sp+8),data)
   if not peek:cursor+=1
  events.append(('read',n,peek,length));uc.reg_write(UC_X86_REG_EAX,length&0xffffffff);ret(16);return
 if a in (0x416190,0x416170,0x416280):
  if a==0x416190:uc.reg_write(UC_X86_REG_EAX,heap);heap+=(get(sp+4)+31)&~15
  if a==0x416280:uc.mem_write(get(sp+4),bytes(uc.mem_read(get(sp+8),get(sp+12))))
  ret();return
 if a==0x416bc0:
  def string(ptr):return bytes(uc.mem_read(ptr,128)).split(b'\0')[0].decode()
  fmt=string(get(sp+8));args=[string(get(sp+12))]
  if '%d' in fmt:args.append(C.c_int32(get(sp+16)).value)
  if fmt.count('%s')==2:args.append(string(get(sp+20)))
  text=(fmt%tuple(args)).encode();uc.mem_write(get(sp+4),text+b'\0');uc.reg_write(UC_X86_REG_EAX,len(text));ret();return
 if a==0x416b20 and get(sp+4)==536:
  events.append(('effect',(7,join_allocated,0,0)));uc.reg_write(UC_X86_REG_EAX,0x190000+join_allocated*536);join_allocated+=1;ret();return
 if a==0x416620 and 0x190000<=get(sp+4)<0x190000+2144:
  events.append(('effect',(8,(get(sp+4)-0x190000)//536,0,0)));ret();return
 if a==0x416b20 and get(sp+4)==20:
  events.append(('effect',(6,death_allocated,0,0)));uc.reg_write(UC_X86_REG_EAX,0x180000+death_allocated*20);death_allocated+=1;ret();return
 if PROJECTILES and a==0x416b20:
  assert get(sp+4)==72 and allocated<1024;events.append(('effect',(1,allocated,0,0)));uc.reg_write(UC_X86_REG_EAX,0x100000+allocated*72);allocated+=1;ret();return
 if PROJECTILES and a==0x416620:
  events.append(('effect',(2,(get(sp+4)-0x100000)//72,0,0)));ret();return
 if PROJECTILES and a==0x407a80:events.append(('effect',(3,get(sp+4),C.c_int32(get(sp+8)).value,C.c_int32(get(sp+12)).value)));return
 if PROJECTILES and a==0x40aae0:
  events.append(('effect',(4,C.c_int32(uc.reg_read(UC_X86_REG_ECX)).value,C.c_int32(get(sp+4)).value,C.c_int32(get(sp+8)).value)));events.append(('effect',(5,get(sp+12),C.c_int32(get(sp+16)).value,0)));ret(16);return
 if a==0x750100:
  put(SHIP+536+0x6c,get(sp+4));ret(4);return
 if a==0x409d70:
  args=tuple(C.c_int32(get(sp+4+4*i)).value for i in range(14));ptr=get(0x429338+args[12]*4);events.append(('weapon',args,bytes(uc.mem_read(ptr,536)).hex()));
  if not PROJECTILES:ret()
  return
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,CLOCK)
r=random.Random(870404);grid=bytearray(1024*1024)
for y in range(20,35):
 for x in range(20,35):
  if x in (20,34) or y in (20,34):grid[y*1024+x]=1
u.mem_write(MAP,bytes(grid));cgrid=(C.c_uint8*len(grid)).from_buffer_copy(grid)
summary={'scenarios':0,'batches':0,'weapon_events':0,'datagrams':0,'ship_bytes_compared':0,'clock_samples':0}
for case in range(80):
 u.reg_write(UC_X86_REG_FPCW,0x027f)
 u.mem_write(MAP,bytes(grid))
 base_ms=100000+case*1000;calls=0;events=[];offset=r.choice([0,1,79,0xffffffff]);initial=bytearray(1072)
 for index,pid in enumerate((7,42)):
  name=('Remote7' if pid==7 else 'Local42').encode()+b'\0';initial[index*536:index*536+len(name)]=name
  values={0x44:pid,0x48:400000,0x4c:400000,0x50:123,0x54:-45,0x5c:1000000,0x60:int(case%7==0),0x64:int(case%11==0),0x6c:(7 if index==1 else -1),0x68:(1 if PROJECTILES and index==0 else 3),0xa0:80}
  for off,value in values.items():struct.pack_into('<i',initial,index*536+off,value)
 lib.critical_join_init(clock,read,send,weapon,bytes(initial),cgrid,offset,2 if PROJECTILES else int(TEAM))
 u.mem_write(NET,bytes(0x4000));u.mem_write(GAME,bytes(32));u.mem_write(SHIP,bytes(initial));u.mem_write(0x429338,bytes(1024*4))
 put(0x429338+7*4,SHIP);put(0x429338+42*4,SHIP+536);put(GAME+8,42);put(GAME+12,1);put(GAME+16,NET)
 initial_state=(C.c_uint32*6)();lib.critical_network_state(initial_state);initial_tick=initial_state[1]
 put(NET,initial_tick);put(NET+4,9999999);put(NET+0x8b0,3);put(NET+0x8ac,offset);put(NET+0x3c54,1000);put(NET+0x14,initial_tick)
 death_allocated=0;heap=0x200000;join_allocated=0;u.mem_write(0x180000,b'\x55'*320);u.mem_write(0x429288,bytes(64))
 for a,n in [(0x424330,16),(0x4287e8,512),(0x428a50,16)]:u.mem_write(a,bytes(n))
 put(NET+0x1bc8,1000);put(0x421034,0);put(0x424348,GAME);put(GAME,0x570100);put(0x57012c,0x411cc0)
 if PROJECTILES:
  allocated=0;u.mem_write(0x100000,b'\x55'*73728);u.mem_write(0x700000,bytes(8192));u.mem_write(0x429748,bytes(2048));u.mem_write(0x4281c9,C.string_at(lib.critical_network_config(),288))
  for a,v in [(0x429f50,0x700000),(0x42de7c,32),(0x429328,2),(0x4218e0,SHIP+536),(0x4289f0,0),(0x422f78,12345),(0x428178,123),(0x42930c,0),(0x42818c,640),(0x4218f4,124),(0x4218f8,177),(0x4218ec,121),(0x4218f0,123),(0x428190,162),(0x428194,164),(0x428198,165),(0x428168,166),(0x42816c,167),(0x428170,168)]:put(a,v)
  put(0x4283e8,SHIP+536);put(0x4283ec,0);put(0x42826d,7);put(0x428211,6000)
  join_allocated=0;u.mem_write(0x190000,b'\x55'*2144);put(0x429338+7*4,0);put(0x429328,1);put(0x429f58,0);put(0x4289f4,0);put(0x423f98,0)
 if TEAM:
  u.mem_write(0x4281c9,C.string_at(lib.critical_network_config(),288));put(0x4218e0,SHIP+536);put(0x429328,2);put(0x429f58,0);put(0x4289f4,0);put(0x4283e8,SHIP);put(0x4283ec,SHIP+536);put(0x423f98,0)
  put(0x422f78,12345);put(0x4292fc,-7);put(0x424348,0x570000);put(0x570000,0x570100);put(0x57012c,0x750100)
 for a in [0x424300,0x424304,0x424308,0x424310]:put(a,0)
 for a,v in [(0x429f48,MAP),(0x429318,42),(0x424340,-1),(0x428271,16),(0x428a08,0)]:put(a,v)
 def position(index):
  p=bytearray(20);p[0]=5;p[1]=r.randrange(40);p[10]=r.choice([7,7,42]);p[15]=r.randrange(32)
  struct.pack_into('<hhhh',p,2,r.randrange(300,560),r.randrange(300,560),r.randrange(-6000,6000),r.randrange(-6000,6000))
  now=((base_ms//10+offset)&0xffffffff)%0x19999999
  struct.pack_into('<I',p,11,(now-r.choice([0,1,79,80,81,200,0xffffffff]))&0xffffffff)
  struct.pack_into('<hH',p,16,r.choice([0,1,40,80]),(r.choice([1,2,5]) if PROJECTILES else r.randrange(1,6))|((r.randrange(3) if PROJECTILES else r.randrange(4))<<3)|(r.randrange(8)<<5)|(r.randrange(4)<<8)|(r.randrange(32)<<10)|(r.randrange(2)<<15))
  return bytes(p)
 def join_packet(name,ship_type):
  p=bytearray(85);p[0]=3;p[1]=7;p[2]=ship_type;p[3:3+len(name)]=name;p[35:40]=b'Squad';struct.pack_into('<I',p,67,1);struct.pack_into('<h',p,79,-1);struct.pack_into('<I',p,81,1);return bytes(p)
 def motion(x,y):
  p=bytearray(20);p[0]=5;p[10]=7;struct.pack_into('<hhhh',p,2,x,y,123,-45);struct.pack_into('<I',p,11,base_ms//10);return bytes(p)
 payloads=[join_packet(b'PilotA',case%4),motion(400,410),bytes([6,42,7])+struct.pack('<hb',50,5),bytes([4,7]),join_packet(b'PilotB',(case+1)%4),motion(430,420),bytes([15])+struct.pack('<ii',7,3),bytes([6,7,42])+struct.pack('<hb',25,7),bytes([4,7])]
 packets=[b'\0\2'+struct.pack('<IH',1000+i,i)+payloads[i] for i in range(9)]
 # Loss is a deliberately absent datagram until a later retry batch; socket
 # reorder/duplicates must not repeat weapon effects or jump reliable gaps.
 phases=[[packets[2],packets[1],packets[2],bytes([16])+struct.pack('<hh',7,42)],[],[packets[0]],
         [packets[0],packets[4],packets[5],bytes([16])+struct.pack('<hh',42,7)],[packets[3]],
         [packets[8],packets[7],packets[7]],[packets[6]],list(reversed(packets))]
 native_calls=calls
 for batch,stream in enumerate(phases):
  cursor=0;calls=native_calls;events=[];put(STACK,RETURN);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_ECX,GAME)
  u.emu_start(0x411d00,RETURN,count=2000000);assert u.reg_read(UC_X86_REG_EIP)==RETURN
  ne=list(events);ncalls=calls;assert cursor==len(stream)
  assert not any(x[0]=='weapon' for x in ne)
  cursor=0;calls=native_calls;events=[];result=lib.critical_network_poll();assert result==1
  assert events==ne,(case,batch,'trace',events,ne)
  assert calls==ncalls and cursor==len(stream);native_calls=calls
  actual=C.string_at(lib.critical_network_ships(),1072);expected=bytes(u.mem_read(SHIP,1072));assert actual==expected,(case,batch,'ship',[(i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
  if TEAM:
   ts=(C.c_int32*6)();lib.critical_network_team_state(ts)
   expected_team=[get(get(0x4283e8)+0x44),get(get(0x4283ec)+0x44),get(0x429f58),get(0x4289f4),get(0x422f78),C.c_int32(get(0x4292fc)).value]
   assert list(ts)==expected_team,(case,batch,'team',list(ts),expected_team)
  if PROJECTILES:
   lib.critical_network_projectiles.argtypes=[C.c_int,C.c_void_p]
   pool=(C.c_uint8*73728)();lib.critical_network_projectiles(0,pool);assert bytes(pool)==bytes(u.mem_read(0x100000,73728)),(case,batch,'projectile pool')
   meta=(C.c_int32*5)();lib.critical_network_projectiles(1,meta);expected_meta=[allocated,get(0x4289f0),get(0x428178),C.c_int32(get(0x422f78)).value,get(0x42930c)];assert list(meta)==expected_meta,(case,batch,'projectile state',list(meta),expected_meta)
   indices=(C.c_int32*512)();lib.critical_network_projectiles(2,indices);expected_indices=[(get(0x429748+4*i)-0x100000)//72 if get(0x429748+4*i) else -1 for i in range(512)];assert list(indices)==expected_indices,(case,batch,'projectile list')
  lib.critical_join_snapshot.argtypes=[C.c_int,C.c_void_p]
  jp=(C.c_uint8*2144)();lib.critical_join_snapshot(0,jp);assert bytes(jp)==bytes(u.mem_read(0x190000,2144)),(case,batch,'join pool')
  jm=(C.c_int32*7)();lib.critical_join_snapshot(1,jm)
  expected_join=[join_allocated,get(0x429328),get(0x429f58),get(0x4289f4),(get(0x429338+7*4)-0x190000)//536 if get(0x429338+7*4) else -1,get(get(0x4283e8)+0x44),get(get(0x4283ec)+0x44) if get(0x429328)>1 else -1];assert list(jm)==expected_join,(case,batch,'registry',list(jm),expected_join)
  lib.critical_death_snapshot.argtypes=[C.c_int,C.c_void_p]
  def snapshot(which,size):
   out=(C.c_uint8*size)();lib.critical_death_snapshot(which,out);return bytes(out)
  assert snapshot(0,320)==bytes(u.mem_read(0x180000,320)),(case,batch,'pickup objects')
  assert struct.unpack('<2i',snapshot(1,8))==(death_allocated,get(0x428a08)),(case,batch,'pickups')
  assert snapshot(2,1048576)==bytes(u.mem_read(MAP,1048576)),(case,batch,'map')
  assert snapshot(3,544)==bytes(u.mem_read(0x424330,16))+bytes(u.mem_read(0x4287e8,512))+bytes(u.mem_read(0x428a50,16)),(case,batch,'notices')
  assert list(struct.unpack('<16i',snapshot(4,64)))==[(get(0x429288+4*i)-0x180000)//20 if get(0x429288+4*i) else -1 for i in range(16)],(case,batch,'pickup list')
  state=(C.c_uint32*6)();lib.critical_network_state(state)
  native=[get(NET+0x3c54),get(NET+0x14),get(0x424304),get(0x424300),get(0x424308),get(0x424310)];assert list(state)==native,(case,batch,'transport',list(state),native)
  summary['batches']+=1;summary['weapon_events']+=sum(x[0]=='weapon' for x in ne);summary['datagrams']+=len(stream);summary['ship_bytes_compared']+=1072;summary['clock_samples']+=sum(x[0]=='clock' for x in ne)
 assert state[0]==1009;lib.critical_network_clear();summary['scenarios']+=1
summary['x87_control_word']='0x027f';summary['native_entries']=native_entries
assert all(native_entries.get(hex(a),0)>0 for a in (0x411d00,0x401920,0x404680,0x414b10,0x411cc0,0x401610))
summary['mode']='join';summary['bridge_sha256']=hashlib.sha256((BRIDGE).read_bytes()).hexdigest()
assert native_entries.get('0x404680')==160 and native_entries.get('0x404340')==160 and native_entries.get('0x404600')==160 and native_entries.get('0x404530')==80
if TEAM:assert native_entries.get('0x404530')==400 and native_entries.get('0x4137c0')==160 and native_entries.get('0x750100')==80
summary['native_sha256']=hashlib.sha256(exe.read_bytes()).hexdigest();summary['sources']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources+[ROOT/'code/tests/critical_network_bridge.c',ROOT/'code/tests/critical_network_death_bridge.c']+sorted((ROOT/'code/src').glob('*.h'))};summary['test_sha256']=hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest();summary['limitations']=['Inbound death notifications execute real damage(-1),audio RNG,pickups,score,Hud;not projectile-induced local death.','Local player prebound;remote joins/leaves/rejoins,team/death via decoder;local authentication/ready transition remains outside this slice.','No public or historical server contacted.']
(OUT/'result.json').write_text(json.dumps(summary,indent=2)+'\n');print('PASS composed native network gameplay',json.dumps({k:v for k,v in summary.items() if isinstance(v,int)}))
