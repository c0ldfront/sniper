"""Stateful original/session sound RNG comparison; only audio device entry hooked."""
from pathlib import Path
import ctypes as C
import hashlib,json,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'research/experiments/critical-parity/rng'
BUILD=OUT/'build';BUILD.mkdir(parents=True,exist_ok=True)
command=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared','-fvisibility=hidden','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','tests/critical_rng_bridge.c','src/audio_logic.c','src/ship_spawn.c','-o',str(BUILD/'bridge.so')]
subprocess.run(command,cwd=ROOT/'code',check=True,capture_output=True)
lib=C.CDLL(str(BUILD/'bridge.so'));lib.critical_rng_reset.argtypes=[C.c_uint32];lib.critical_rng_step.argtypes=[C.POINTER(C.c_int32)]*2
exe=ROOT/'code/build/assets/files/SNIPER.EXE';pe=pefile.PE(str(exe))
assert hashlib.sha256(exe.read_bytes()).hexdigest()=='8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0'
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pe.get_memory_mapped_image())
put=lambda a,v:u.mem_write(a,struct.pack('<I',v&0xffffffff))
get=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
signed=lambda v:C.c_int32(v).value
rng_addr=struct.unpack('<I',pe.get_data(0x165f1,4))[0]
events=[];visits={0x407a80:0,0x4165f0:0,0x40aae0:0}
def hook(uc,a,size,user):
    if a in visits:visits[a]+=1
    if a!=0x40aae0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    events.append([signed(uc.reg_read(UC_X86_REG_ECX))]+[signed(get(sp+i)) for i in (4,8,12,16)])
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+20)
u.hook_add(UC_HOOK_CODE,hook)
def call(addr,args=(),this=0):
    put(0x800000,0x540000)
    for i,v in enumerate(args):put(0x800004+i*4,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,this)
    u.emu_start(addr,0x540000,count=2000)
    assert u.reg_read(UC_X86_REG_EIP)==0x540000
    return u.reg_read(UC_X86_REG_EAX)
r=random.Random(871995);trace=hashlib.sha256();cases=0;gated=0;requests=0
for stream in range(32):
    seed=r.getrandbits(32);put(rng_addr,seed);lib.critical_rng_reset(seed)
    for step in range(128):
        lx=r.randrange(-2000,16000)*1000+r.randrange(-999,1000)
        ly=r.randrange(-2000,16000)*1000+r.randrange(-999,1000)
        x=int(lx/1000)+r.choice([-801,-800,-1,0,1,800,801]);y=int(ly/1000)+r.choice([-801,-800,-1,0,1,800,801])
        if step%13==0:x=y=-1
        width=r.choice([320,640,800,1024,1280,1600]);spread=r.choice([1,100,500])
        present=int(step%11!=0);observer=int(step%3!=0);mode=step%3;draws=step%5
        args=[x,y,lx,ly,width,spread,present,observer,mode,draws]
        for a,v in [(0x4218e0,0x590000 if present else 0),(0x590048,lx),(0x59004c,ly),(0x42818c,width),(0x4282a5,spread)]:put(a,v)
        events.clear()
        if mode==1:call(0x40aae0,(0,0,0,0),148)
        else:call(0x407a80,(134 if mode==2 else 162,x,y))
        expected=[0]*12
        if observer and events:expected[:6]=[len(events)]+events[0]
        expected[6]=signed(get(rng_addr));gated+=int(mode!=1 and not events);requests+=len(events)
        for i in range(draws):expected[7+i]=signed(call(0x4165f0))
        expected[11]=signed(get(rng_addr))
        out=(C.c_int32*12)();lib.critical_rng_step((C.c_int32*10)(*args),out)
        assert list(out)==expected,dict(stream=stream,step=step,args=args,expected=expected,actual=list(out))
        trace.update(struct.pack('<22i',*args,*expected));cases+=1
summary={'result':'PASS','streams':32,'events':cases,'native_audio_requests':requests,'positional_gates':gated,'native_entry_visits':{hex(k):v for k,v in visits.items()},'trace_sha256':trace.hexdigest(),'command':'python3 code/tests/critical_rng_native_test.py','scope':'Original407a80 and4165f0 vs private production session sound/prize/impact callbacks; persistent RNG and subsequent draws after every event. Hook only final audio device entry40aae0. No acoustic output or full combat frame claim. Positive pitch spread and supported display widths.','build_command':command,'sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [exe,Path(__file__),ROOT/'code/tests/critical_rng_bridge.c',ROOT/'code/src/offline_session.c',ROOT/'code/src/audio_logic.c',ROOT/'code/src/ship_spawn.c',BUILD/'bridge.so']}}
(OUT/'result.json').write_text(json.dumps(summary,indent=2)+'\n')
message=f'PASS {cases} stateful audio/RNG events across32 streams; {gated} positional gates, {requests} native audio requests; subsequent gameplay draws identical'
(OUT/'result.txt').write_text(message+'\n');print(message)
