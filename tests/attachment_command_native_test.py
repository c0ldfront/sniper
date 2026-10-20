"""F7 branch with actual offline attach callback and ship-state mutation."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Notices(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
lib=C.CDLL(str(root/'code/build/libattachment_command_test.so'))
lib.test_attachment_command.argtypes=[C.POINTER(Ship),C.POINTER(Notices),C.c_int32,C.POINTER(C.c_int32)]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
for a,v in [(0x4218e0,0x500000),(0x429318,0),(0x429328,8),(0x424348,0x520000),(0x520000,0x420008),(0x4218e8,87)]:put(a,v)
trace=[]
def hook(uc,a,size,user):
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x411440:trace.append(1)
    elif a==0x411620:trace.extend([3,get(sp+4),get(0x50005c)])
    elif a==0x40aae0:
        assert uc.reg_read(UC_X86_REG_ECX)==87
        assert [get(sp+i) for i in [4,8,12,16]]==[0,0,0,0]
        trace.extend([2,get(0x50005c)])
        uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+20)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87);outcomes=set()
for case in range(1200):
    ships=(Ship*8)();n=Notices()
    for i,s in enumerate(ships):
        for j in range(134):s.word[j]=r.randrange(-2147483648,2147483648)
        s.word[0x44//4]=i;s.word[0x6c//4]=-1;s.word[0x40//4]=case%5;s.word[0x68//4]=2
        put(0x429338+4*i,0x500000+536*i);put(0x4283e8+4*i,0x500000+536*i)
    selected=case%10
    local=ships[0];capacity=r.choice([-2147483648,-5,0,1,500000,1000000,2147483647])
    local.word[0xc8//4]=local.word[0x5c//4]=capacity
    mode=(case//10)%8
    if mode==0:local.word[0x6c//4]=2
    elif mode==1:ships[3].word[0x6c//4]=0
    elif mode==2:local.word[0x5c//4]=capacity^1
    elif mode==3 and selected<8:ships[selected].word[0x68//4]=3
    elif mode==4 and selected<8:ships[selected].word[0x40//4]=4
    elif mode==5 and selected<8:ships[selected].word[0x6c//4]=6
    C.memset(C.addressof(n),0x5a,C.sizeof(n))
    for i in range(4):n.ticks[i]=r.choice([-1,0,1,300])
    u.mem_write(0x500000,bytes(ships));put(0x429f58,selected)
    u.mem_write(0x424330,bytes(n.ticks));u.mem_write(0x4287e8,bytes(n.text));u.mem_write(0x428a50,bytes(n.color))
    trace.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x405986,0x405d56,count=200000)
    result=(C.c_int32*8)();length=lib.test_attachment_command(ships,C.byref(n),selected,result)
    assert list(result)[:length]==trace,(case,list(result)[:length],trace)
    assert bytes(ships)==bytes(u.mem_read(0x500000,C.sizeof(ships))),case
    assert bytes(n.ticks)==bytes(u.mem_read(0x424330,16)),case
    assert bytes(n.text)==bytes(u.mem_read(0x4287e8,512)),case
    assert bytes(n.color)==bytes(u.mem_read(0x428a50,16)),case
    outcomes.add(tuple(trace[:1]))
assert outcomes=={(),(1,),(2,),(3,)},outcomes
print('PASS1200 F7 cases: native offline attach, full ships/notices, detach, child-release, validation precedence, parent targeting, signed energy cost and sound order')
