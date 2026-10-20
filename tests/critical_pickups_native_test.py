"""Persistent pickup contact/award, movement, weapon decisions, expiry and warp.
Native machine state and portable production state are never resynchronized
between actions. Only environmental pickup insertion is supplied to both.
Projectile creation is an explicit captured boundary, not simulated here.
"""
import ctypes as C
import pathlib, struct, json, subprocess, hashlib
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'research/experiments/critical-parity/pickups';BUILD=OUT/'build'
BUILD.mkdir(parents=True,exist_ok=True)
SOURCES=['notices.c','attachment.c','ship_update_order.c','prize.c','ship_world.c',
 'ship_update.c','ship_energy.c','ship_spawn.c','ship_reset.c','movement.c',
 'local_config.c','offline_pickups.c','pickup_collect.c','pickup_lifecycle.c',
 'offline_generation.c','weapon_action.c','ship_flight.c','flight.c',
 'flight_input.c','flight_boost.c','flight_math.c','ship_effect.c']
source_paths=[ROOT/'code/src'/p for p in SOURCES]
bridge=ROOT/'code/tests/critical_pickups_bridge.c'
subprocess.run(['cc','-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Isrc','-fPIC','-shared',
 *map(str,source_paths),str(bridge),'-o',str(BUILD/'libcritical_pickups.so')],cwd=ROOT/'code',check=True)
exe=ROOT/'code/build/assets/files/SNIPER.EXE'
assert hashlib.sha256(exe.read_bytes()).hexdigest()=='8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0'
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Pickup(C.Structure):_fields_=[(n,C.c_int32) for n in ['age','x','y','lifetime','value']]
class Hud(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
Sound=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
Message=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32)
Event=C.CFUNCTYPE(None,C.c_int,C.c_int)
Weapon=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int32))
ce=[];ne=[];nused=set();weapons=[];flight_keys=0
@Sound
def sound(_,v):ce.append(('sound',v))
@Message
def message(_,text,ticks,color):ce.append(('message',text.decode(),ticks,color))
@Event
def event(kind,i):ce.append(('alloc' if kind else 'free',i))
@Weapon
def weapon(_,args):ce.append(('weapon',tuple(args[i] for i in range(14))))
counts={a:0 for a in [0x413fa0,0x411560,0x404910,0x415030,0x4137c0,0x4165f0,0x407b50]}
def hook(uc,a,size,data):
    if a in counts:counts[a]+=1
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x407b50:
        text=bytes(u.mem_read(get(sp+4),128)).split(b'\0')[0].decode()
        ne.append(('message',text,get(sp+8),get(sp+12)));return # Run native HUD insertion.
    if a not in [0x820000,0x820010,0x416b20,0x416620,0x40aae0,0x40aab0,0x409d70]:return
    clean=0;value=0
    if a==0x820000:value=10000
    elif a==0x820010:
        clean=4;value=0x8000 if flight_keys & {38:1,40:2,37:4,39:8,16:16}.get(get(sp+4),0) else 0
    elif a==0x416b20:
        assert get(sp+4)==20
        i=next(i for i in range(16) if i not in nused);nused.add(i);value=0x530000+i*32;ne.append(('alloc',i))
    elif a==0x416620:
        i=(get(sp+4)-0x530000)//32;nused.remove(i);ne.append(('free',i))
    elif a in [0x40aae0,0x40aab0]:
        handle=uc.reg_read(UC_X86_REG_ECX)
        if 0x550000<=handle<=0x550200:handle=200+((handle-0x550000)//256)*2+int(a==0x40aae0)
        ne.append(('sound',handle));clean=16 if a==0x40aae0 else 0
    else:ne.append(('weapon',tuple(C.c_int32(get(sp+4+i*4)).value for i in range(14))))
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
lib=C.CDLL(str(BUILD/'libcritical_pickups.so'))
lib.critical_pickups_begin.argtypes=[C.POINTER(Ship),C.c_void_p,C.c_uint32,Sound,Message,Event,Weapon]
lib.critical_pickups_add.argtypes=[C.c_int32]*4
lib.critical_pickups_frame.argtypes=[C.c_int32]
lib.critical_pickups_fire.argtypes=[C.c_int32,C.c_void_p]
lib.critical_pickups_snapshot.argtypes=[C.c_void_p]*4
lib.critical_pickups_flight.argtypes=[C.c_uint32,C.c_int32,C.c_void_p]
config=(C.c_int32*72)();lib.sniper_local_config(config)
for a in [0x428289,0x42828d]:config[(a-0x4281c9)//4]=250
u.mem_write(0x4281c9,bytes(config))
for a,v in [(0x42190c,101),(0x4218ec,102),(0x4218e8,103),(0x4353f4,0x820000),
            (0x4354b8,0x820010),(0x429f48,0x600000),(0x429f50,0x700000),(0x42de7c,32),
            (0x4218e0,0x500000),(0x429318,0),(0x424340,0),(0x429328,1),
            (0x42de80,0),(0x424324,0),(0x424348,0x540000),(0x540000,0x420008)]:put(a,v)
for slot in range(3):put(0x4218fc+4*slot,0x550000+slot*256)
for a,v in [(0x42930c,1),(0x42e144,1),(0x422278,0),(0x4292e8,0)]:put(a,v)
grid=(C.c_uint8*(1024*1024))();coverage=(C.c_uint8*8192)();result=(C.c_uint32*5)();out=(Pickup*16)();hud=Hud()
trace=[];scenario=0;index=0

def run(a,args=(),end=0x810000):
    for i,v in enumerate([0x810000,*args]):put(0x800000+4*i,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000)
    u.emu_start(a,end,count=2000000)
    assert u.reg_read(UC_X86_REG_EIP)==end

def check(action):
    global index
    lib.critical_pickups_snapshot(result,out,coverage,C.byref(hud))
    actual=bytes(u.mem_read(0x500000,536))
    differences=[hex(i) for i in range(0,536,4) if bytes(ship)[i:i+4]!=actual[i:i+4]]
    assert not differences,(scenario,index,action,'ship',differences)
    assert list(result)==[get(a) for a in [0x422f78,0x4292fc,0x428a08,0x429300,0x4283d8]],(scenario,index,action,'globals',list(result))
    assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),(scenario,index,action,'map')
    assert bytes(coverage)==bytes(u.mem_read(0x700000,8192)),(scenario,index,action,'coverage')
    for i in range(result[2]):assert bytes(out[i])==bytes(u.mem_read(get(0x429288+i*4),20)),(scenario,index,'pickup',i)
    for data,a in [(hud.ticks,0x424330),(hud.text,0x4287e8),(hud.color,0x428a50)]:
        assert bytes(data)==bytes(u.mem_read(a,C.sizeof(data))),(scenario,index,action,'hud',hex(a))
    assert ce==ne,(scenario,index,action,'events',ce,ne)
    trace.append({'scenario':scenario,'step':index,'action':action,'rng':int(result[0]),'pickups':int(result[2]),
                  'position':list(ship.word)[18:20],'velocity':list(ship.word)[20:22], 'thrust':ship.word[0xe0//4],'speed':ship.word[0xe4//4], 'energy':ship.word[0x5c//4],
                  'gun':ship.word[0xd0//4],'bomb':ship.word[0xd4//4],
                  'super':ship.word[0x10c//4],'shield':ship.word[0x108//4], 'events':list(ce)})
    index+=1;ce.clear();ne.clear()

def add(code,life=300,far=False):
    x,y=(800,800) if far else ((ship.word[0x48//4]+ship.word[0x50//4]+(14000 if ship.word[0x50//4]>=0 else -14000))//16000,ship.word[0x4c//4]//16000)
    assert 0<=x<1024 and 0<=y<1024
    run(0x404f10,(x,y,code,life));lib.critical_pickups_add(x,y,code,life);check(('add',code,x,y,life))

def frame(ticks=10):
    run(0x413fa0,(ticks,));put(0x4218c8,ticks);u.reg_write(UC_X86_REG_EAX,0)
    run(0x4021d5,end=0x402237);run(0x4022f2,end=0x402310)
    put(0x429300,get(0x429300)-ticks);put(0x4283d8,get(0x4283d8)-ticks)
    lib.critical_pickups_frame(ticks);check(('frame',ticks))

def flight(keys=9,ticks=5):
    global flight_keys
    flight_keys=keys;put(0x4218c8,ticks);put(0x800010,0)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_FPCW,0x27f)
    u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    u.emu_start(0x405ffa,0x406332,count=10000)
    extra=(C.c_int32*2)();lib.critical_pickups_flight(keys,ticks,extra)
    assert list(extra)==[C.c_int32(get(a)).value for a in [0x42833c,0x800010]]
    flight_keys=0;check(('flight',keys,ticks))

def fire(action):
    fields=(C.c_int32*6)();u.reg_write(UC_X86_REG_ESP,0x800000)
    for off in [0x20,0x1c,0x14,0x10,0x18]:put(0x800000+off,0)
    u.reg_write(UC_X86_REG_EBP,1);u.reg_write(UC_X86_REG_EDI,action);u.reg_write(UC_X86_REG_EBX,0)
    u.emu_start(0x406431,0x406922,count=100000)
    lib.critical_pickups_fire(action,fields)
    expected=[C.c_int32(u.reg_read(UC_X86_REG_ESI)).value]+[C.c_int32(get(0x800000+off)).value for off in [0x20,0x1c,0x14,0x10,0x18]]
    assert list(fields)==expected,(scenario,index,'weapon fields',list(fields),expected)
    if any(e[0]=='weapon' for e in ce):weapons.append((scenario,action,ship.word[0xd0//4],ship.word[0xd4//4]))
    check(('fire',action))

for scenario in range(4):
    u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    ship=Ship()
    for off,v in [(0x40,scenario),(0x44,0),(0x48,400000),(0x4c,400000),
      (0x50,100),(0x54,100),(0x5c,600000),(0x68,7),(0x6c,-1),
      (0x74,1000),(0xc4,166),(0xc8,1000000),(0xcc,160),(0xe0,10),(0xe4,2000),(0xd0,1)]:ship.word[off//4]=v
    C.memset(grid,0,len(grid));u.mem_write(0x600000,bytes(grid));u.mem_write(0x500000,bytes(ship));u.mem_write(0x700000,bytes(8192))
    for a,size in [(0x424330,16),(0x4287e8,512),(0x428a50,16)]:u.mem_write(a,bytes(size))
    for a,v in [(0x422f78,87+scenario),(0x4292fc,-7),(0x428a08,0),(0x4289f0,0),(0x429300,0),(0x4283d8,0),(0x42833c,0)]:put(a,v)
    nused.clear();ce.clear();ne.clear();index=0
    lib.critical_pickups_begin(C.byref(ship),grid,87+scenario,sound,message,event,weapon)
    add(1,35,True) # Uncollected pickup expires while nearby prizes are consumed.
    fire(0);flight()
    for code in [11,12,8,9,15,17,18,17,18]:
        awards=counts[0x415030];old_thrust=ship.word[0xe0//4];old_speed=ship.word[0xe4//4]
        add(code);frame(10)
        assert counts[0x415030]==awards+1,('uncollected',scenario,code)
        if code==11:assert ship.word[0xe0//4]>old_thrust
        if code==12:assert ship.word[0xe4//4]>old_speed
        if code==17:assert ship.word[0x10c//4]>0
        if code==18:assert ship.word[0x108//4]>0
        if code in [11,12,17]:flight()
        if code in [8,9,17]:
            for _ in range(4):frame(50)
            fire(int(code==9))
    for _ in range(8):frame(50)
    assert ship.word[0x10c//4]<=0 and ship.word[0x108//4]<=0
    add(-8);frame(10)
    for _ in range(4):frame(50)
    fire(0)
    before=(ship.word[0x48//4],ship.word[0x4c//4]);rng_before=int(result[0]);add(7);frame(10)
    assert before!=(ship.word[0x48//4],ship.word[0x4c//4]) and result[0]!=rng_before,'warp did not run'
    for _ in range(5):frame(10)
    assert result[2]==0, (scenario, list(result), [(out[i].x,out[i].y,out[i].value,out[i].lifetime) for i in range(result[2])],trace[-8:])
assert all(counts.values()),counts
assert counts[0x415030]==44 and counts[0x4137c0]==4,counts
assert len(weapons)>=16,weapons
output=BUILD/'trace.json'
output.write_text(json.dumps({'calls':{hex(k):v for k,v in counts.items()},'weapon_actions':weapons,'steps':trace},indent=2)+'\n')
print(f'PASS {len(trace)} persistent actions across4 ship types: contact/award, upgraded flight, weapon decisions, expiration, warp; ship/map/list/RNG/coverage/HUD/callbacks match every action')
print('Native routine calls:',{hex(k):v for k,v in counts.items()})

manifest={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [exe,bridge,pathlib.Path(__file__),*source_paths,output,BUILD/'libcritical_pickups.so']}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(OUT/'result.json').write_text(json.dumps({'status':'pass','actions':len(trace),'scenarios':4,
 'native_calls':{hex(k):v for k,v in counts.items()},'weapon_boundary_actions':len(weapons),
 'boundaries':['environment inserts pickups; generation not in this chain',
 'weapon arguments captured before projectile creation', 'speed-exempt native flight branch',
 'direct prize/flight audio at final device boundary; no positional407a80 sound in this scenario']},indent=2)+'\n')
