"""Original spectator input tail with backend hooks and full queued cleanup."""
import ctypes as C,pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Input(C.Structure):_fields_=[('local',C.POINTER(Ship))]+[(x,C.c_int32) for x in ['key','repeat','left','right','joystick_enabled','local_id']]+[('character',C.c_uint8),('joystick',C.c_uint32*13)]+[(x,C.c_void_p) for x in ['damage','poll','stage','user']]
Pos=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int32));Watch=C.CFUNCTYPE(None,C.c_void_p,C.c_int32)
class State(C.Structure):_fields_=[('input',C.POINTER(Input)),('ordered',C.POINTER(C.POINTER(Ship)))]+[(x,C.c_int32) for x in ['count','selected','tick','last_tick','watched']]+[('position',Pos),('watch',Watch),('user',C.c_void_p)]
ce=[];ne=[];mutate=0
@Pos
def pos(user,args):
    ce.append(('pos',tuple(args[i] for i in range(13))))
    if mutate:s.tick=C.c_int32(s.tick+7).value;inp.key=17
@Watch
def watch(user,id):ce.append(('watch',id,s.watched))
ships=[Ship() for _ in range(3)];ordered=(C.POINTER(Ship)*3)(*[C.pointer(x) for x in ships]);inp=Input();s=State(C.pointer(inp),ordered,3,0,0,0,-1,pos,watch,None)
lib=C.CDLL(str(root/'code/build/libspectator_control.so'));lib.sniper_spectator_control.argtypes=[C.POINTER(State)]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
for a,v in [(0x424348,0x520000),(0x520000,0x520100),(0x52010c,0x530000),(0x520120,0x530100)]:put(a,v)
def hook(uc,a,size,user):
    if a not in [0x530000,0x530100]:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x530000:
        ne.append(('pos',tuple(get(sp+4+i*4) for i in range(13))));clean=52
        if mutate:put(0x4218cc,get(0x4218cc)+7);put(0x422248,17)
    else:ne.append(('watch',get(sp+4),get(0x424340)));clean=4
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook);r=random.Random(87)
for case in range(1800):
    for i,ship in enumerate(ships):
        ship.word[0x40//4]=r.choice([-1,0,3,4,5]);ship.word[0x48//4]=r.choice([-1,-2,0,1234]);ship.word[0x44//4]=i+7
        u.mem_write(0x500000+i*0x1000,bytes(ship));put(0x4283e8+i*4,0x500000+i*0x1000)
    s.count=r.randrange(4);s.selected=r.randrange(5);s.tick=r.choice([-2147483648,0,399,400,401,2147483647]);s.last_tick=r.choice([-1,0,400,2147483647]);s.watched=r.choice([-1,3,7]);inp.key=r.choice([0,9,17,123]);inp.repeat=123;inp.character=65;inp.left=1;inp.right=2
    for a,v in [(0x429328,s.count),(0x429f58,s.selected),(0x4218cc,s.tick),(0x4281c0,s.last_tick),(0x424340,s.watched),(0x422248,inp.key),(0x42224c,inp.repeat),(0x4222a4,1),(0x4222a8,2)]:put(a,v)
    u.mem_write(0x422244,b'A');ce.clear();ne.clear();mutate=case%3==0
    u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x405ed9,0x406a9f,count=100000)
    lib.sniper_spectator_control(C.byref(s))
    assert ce==ne,(case,ce,ne)
    assert s.last_tick==get(0x4281c0) and s.watched==get(0x424340)
    assert not any([inp.key,inp.repeat,inp.character,inp.left,inp.right])
    assert not any(get(a) for a in [0x422248,0x42224c,0x4222a4,0x4222a8]) and bytes(u.mem_read(0x422244,1))==b'\0'
print('PASS1800 original spectator cases:heartbeat boundary/wrap,live callbacks,Tab/Ctrl target gates,watch ordering and queued-input cleanup')
