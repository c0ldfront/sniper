"""Original full steering/thrust block; speed limiting separately hardware-tested."""
import ctypes as C
import pathlib,struct,random,sys
AUDIO="--audio" in sys.argv
SHIP_MODE="--ship" in sys.argv or AUDIO
POLL="--poll" in sys.argv or AUDIO
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
SHIP=0x500000;STACK=0x510000;KEY=0x520000
class Control(C.Structure):_fields_=[(n,C.c_int32) for n in ['heading','rotation_rate','lock_ticks','attached_id']]
class Input(C.Structure):_fields_=[(n,C.c_uint32) for n in ['keys','joystick_enabled','joystick_x','joystick_y']]
class Config(C.Structure):_fields_=[(n,C.c_int32) for n in ['cost','acceleration','maximum','type0_acceleration_bonus','type1_speed_bonus']]
class Flight(C.Structure):_fields_=[('control',Control)]+[(n,C.c_int32) for n in ['type','energy','acceleration','maximum','vx','vy','exhaust_ticks']]
class Effects(C.Structure):_fields_=[(n,C.c_int32) for n in ['thrust','boosted','exhaust','sound_count']]+[('sound_slot',C.c_int32*3),('sound_play',C.c_int32*3)]
if AUDIO:
    import subprocess
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC']+
        [str(ROOT/'code/src'/p) for p in ['ship_flight.c','flight.c','flight_input.c','flight_boost.c','flight_math.c','ship_effect.c']]+
        ['-o',str(ROOT/'code/build/libship_flight_audio_test.so')],check=True)
lib=C.CDLL(str(ROOT/('code/build/libship_flight_audio_test.so' if AUDIO else 'code/build/libflight.so')))
lib.sniper_flight_step.argtypes=[C.POINTER(Flight),C.POINTER(Input),C.c_uint32,C.POINTER(Config),C.c_int32,C.c_int,C.c_int,C.POINTER(Effects)]
lib.sniper_flight_step.restype=None
lib.sniper_flight_step_scratch.argtypes=lib.sniper_flight_step.argtypes+[C.POINTER(C.c_int32)]
lib.sniper_flight_step_scratch.restype=None
if SHIP_MODE:
    ship_lib=C.CDLL(str(ROOT/('code/build/libship_flight_audio_test.so' if AUDIO else 'code/build/libship_flight.so')))
    ship_lib.sniper_ship_flight.argtypes=[C.POINTER(C.c_int32),C.POINTER(Input),C.c_uint32,C.POINTER(Config),C.c_int32,C.c_int,C.c_int,C.POINTER(C.c_int32),C.POINTER(Effects),C.POINTER(C.c_int32)]
    ship_lib.sniper_ship_flight.restype=None
KeyFn=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_int32)
lib.sniper_flight_step_polled.argtypes=lib.sniper_flight_step_scratch.argtypes+[KeyFn,C.c_void_p]
lib.sniper_flight_step_polled.restype=None
if SHIP_MODE:
    ship_lib.sniper_ship_flight_polled.argtypes=ship_lib.sniper_ship_flight.argtypes+[KeyFn,C.c_void_p]
    ship_lib.sniper_ship_flight_polled.restype=None
AudioFn=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int)
native_audio=[];portable_audio=[]
@AudioFn
def audio_event(user,slot,play):
    portable_audio.append((slot,play,bytes(raw),timer.value,scratch.value))
if AUDIO:
    ship_lib.sniper_ship_flight_polled_audio.argtypes=ship_lib.sniper_ship_flight_polled.argtypes+[AudioFn,C.c_void_p]
    ship_lib.sniper_ship_flight_polled_audio.restype=None
queries=[];recovered_queries=[];responses=[]
@KeyFn
def poll_key(user,vk):
    value=responses[len(recovered_queries)]
    recovered_queries.append(vk)
    return value
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def signed(a):return C.c_int32(get(a)).value
put(0x4354b8,KEY);put(0x4218e0,SHIP);put(0x42930c,1)
for slot in range(3):put(0x4218fc+4*slot,0x530000+slot*256)
key_bits={0x26:1,0x28:2,0x25:4,0x27:8,0x10:16}
keys=0;events=[]
def hook(uc,addr,size,data):
    if addr not in (KEY,0x40aae0,0x40aab0,0x415ac0):return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if addr==KEY:
        vk=get(sp+4)
        value=responses[len(queries)] if POLL else (0x8000 if keys & key_bits[vk] else 0)
        queries.append(vk)
        uc.reg_write(UC_X86_REG_EAX,value);clean=4
    elif addr==0x415ac0:
        events.append(('exhaust',))
        if SHIP_MODE:return
    else:
        slot=(uc.reg_read(UC_X86_REG_ECX)-0x530000)//256
        if addr==0x40aae0:
            assert [get(sp+i) for i in (4,8,12,16)]==[0,0,1,0]
            clean=16
        events.append(('sound',slot,int(addr==0x40aae0)))
        if AUDIO:native_audio.append((slot,int(addr==0x40aae0),bytes(u.mem_read(SHIP,536)),signed(0x42833c),signed(STACK+0x10)))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(871);count=0;boost_count=0
for i in range(3000):
    speed_exempt=i%2 if AUDIO else 1;put(0x42930c,speed_exempt)
    keys=i%32;enabled=rng.randrange(2);jx=rng.randrange(65536);jy=rng.randrange(65536)
    buttons=rng.choice([0,4]) if enabled else 0
    ticks=rng.randrange(51);cost=rng.randrange(20);energy=rng.choice([cost*ticks-1,cost*ticks,cost*ticks+1,100000])
    state=Flight(Control(rng.randrange(40000),rng.choice([33,66,99]),rng.choice([0,0,1]),rng.choice([-1,-1,2])),rng.randrange(4),energy,rng.randrange(1,150),rng.randrange(100,5000),rng.randrange(-3000,3001),rng.randrange(-3000,3001),rng.randrange(-5,20))
    inp=Input(keys,enabled,jx,jy);cfg=Config(cost,200,6000,25,1000);eff=Effects();engine=rng.randrange(2)
    if SHIP_MODE:
        raw=(C.c_int32*134)(*[rng.randrange(-2**31,2**31) for _ in range(134)])
        raw[0x214//4]=i%12
        raw[0x48//4]=rng.randrange(-10000,16384000)
        raw[0x4c//4]=rng.randrange(-10000,16384000)
        u.mem_write(SHIP,bytes(raw))
    for off,val in [(0x58,state.control.heading),(0xcc,state.control.rotation_rate),(0x104,state.control.lock_ticks),(0x6c,state.control.attached_id),(0x40,state.type),(0x5c,state.energy),(0xe0,state.acceleration),(0xe4,state.maximum),(0x50,state.vx),(0x54,state.vy)]:put(SHIP+off,val)
    for a,v in [(0x422278,enabled),(0x4292d0,jx),(0x4292d4,jy),(0x4292e8,buttons),(0x4218c8,ticks),(0x428285,cfg.cost),(0x428231,cfg.acceleration),(0x428235,cfg.maximum),(0x428259,cfg.type0_acceleration_bonus),(0x42825d,cfg.type1_speed_bonus),(0x42833c,state.exhaust_ticks),(0x42e144,engine)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x37f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    scratch=C.c_int32(rng.randrange(-2147483648,2147483648));put(STACK+0x10,scratch.value)
    if SHIP_MODE:raw=(C.c_int32*134).from_buffer_copy(bytes(u.mem_read(SHIP,536)))
    queries.clear();recovered_queries.clear()
    responses=[rng.choice([0,1,0x8000,0x8001]) for _ in range(6)] if POLL else []
    native_audio.clear();portable_audio.clear()
    events.clear();u.emu_start(0x405ffa,0x406332,count=2000)
    if SHIP_MODE:
        timer=C.c_int32(state.exhaust_ticks)
        fn=ship_lib.sniper_ship_flight_polled if POLL else ship_lib.sniper_ship_flight
        extra=[poll_key,None] if POLL else []
        if AUDIO:fn=ship_lib.sniper_ship_flight_polled_audio;extra += [audio_event,None]
        fn(raw,C.byref(inp),buttons,C.byref(cfg),ticks,engine,speed_exempt,C.byref(timer),C.byref(eff),C.byref(scratch),*extra)
        assert bytes(raw)==bytes(u.mem_read(SHIP,536)),(i,[(j,a,b) for j,(a,b) in enumerate(zip(raw,struct.unpack('<134i',u.mem_read(SHIP,536)))) if a!=b])
        assert timer.value==signed(0x42833c)
        if AUDIO:assert native_audio==portable_audio,(i,native_audio,portable_audio)
        state.control.heading=raw[0x58//4];state.energy=raw[0x5c//4]
        state.vx=raw[0x50//4];state.vy=raw[0x54//4];state.exhaust_ticks=timer.value
    else:
        fn=lib.sniper_flight_step_polled if POLL else lib.sniper_flight_step_scratch
        extra=[poll_key,None] if POLL else []
        fn(C.byref(state),C.byref(inp),buttons,C.byref(cfg),ticks,engine,speed_exempt,C.byref(eff),C.byref(scratch),*extra)
    if POLL:assert queries==recovered_queries,(i,queries,recovered_queries)
    assert scratch.value==signed(STACK+0x10),(i,scratch.value,signed(STACK+0x10))
    actual=[state.control.heading,state.energy,state.vx,state.vy,state.exhaust_ticks]
    expected=[signed(SHIP+off) for off in (0x58,0x5c,0x50,0x54)]+[signed(0x42833c)]
    assert actual==expected,(i,keys,actual,expected)
    wanted=[('sound',eff.sound_slot[j],eff.sound_play[j]) for j in range(eff.sound_count)]
    if eff.exhaust:wanted.append(('exhaust',))
    assert wanted==events,(i,wanted,events)
    boost_count+=eff.boosted;count+=1
print(f'{count} complete steering/thrust calls match original state, fragment-count scratch and sound/exhaust sequences; {boost_count} boosts applied')

if SHIP_MODE:print("Whole ship bytes and original exhaust routine verified, including ring wrap")

if POLL:print("Live key query order and independently changing Shift values verified")

if AUDIO:print("Per-event audio timing matches complete536-byte ship,exhaust timer and weapon scratch at every original sound call")
