"""404fe0 and actual411620 wrapper; only sprintf is a controlled CRT seam."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2];u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Hud(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
def hook(uc,a,size,data):
    if a!=0x416bc0:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    assert bytes(uc.mem_read(get(sp+8),20)).split(b'\0',1)[0]==b'Player Attached: %s'
    name=bytes(uc.mem_read(get(sp+12),32)).split(b'\0',1)[0];text=b'Player Attached: '+name
    uc.mem_write(get(sp+4),text+b'\0');uc.reg_write(UC_X86_REG_EAX,len(text))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
lib=C.CDLL(str(ROOT/'code/build/libattachment.so'))
lib.sniper_attachment.argtypes=[C.POINTER(C.POINTER(Ship)),C.c_int32,C.POINTER(Hud),C.c_int32,C.c_int32]
rng=random.Random(404);notices=0
for case in range(600):
    ship=Ship();name=('Pilot'+str(case)+'x'*rng.randrange(20)).encode();C.memmove(C.byref(ship),name+b'\0',len(name)+1)
    ship.word[0x6c//4]=rng.randrange(-1,5);hud=Hud();C.memset(C.byref(hud),0x5a,C.sizeof(hud))
    for i in range(4):hud.ticks[i]=rng.choice([-1,0,1,600])
    if case%5==0:
        for i in range(4):hud.ticks[i]=1
    table=(C.POINTER(Ship)*1024)();player=0 if case%2 else 3;present=case%7!=0
    if present:table[player]=C.pointer(ship)
    u.mem_write(0x429338,bytes(4096));put(0x429338+player*4,0x500000 if present else 0)
    local=case%3;target=rng.choice([-1,0,1,2,3]);put(0x429318,local);u.mem_write(0x500000,bytes(ship))
    u.mem_write(0x424330,bytes(hud.ticks));u.mem_write(0x4287e8,bytes(hud.text));u.mem_write(0x428a50,bytes(hud.color))
    put(0x510000,0x520000)
    if case%2:put(0x510004,target);entry=0x411620
    else:put(0x510004,player);put(0x510008,target);entry=0x404fe0
    u.reg_write(UC_X86_REG_ESP,0x510000);u.emu_start(entry,0x520000,count=10000)
    lib.sniper_attachment(table,local,C.byref(hud),player,target)
    assert bytes(ship)==bytes(u.mem_read(0x500000,536))
    assert bytes(hud.ticks)==bytes(u.mem_read(0x424330,16))
    assert bytes(hud.text)==bytes(u.mem_read(0x4287e8,512))
    assert bytes(hud.color)==bytes(u.mem_read(0x428a50,16))
    notices+=present and target==local
print(f'600 attachment cases match ship and all HUD bytes, including offline wrapper; {notices} notices')
