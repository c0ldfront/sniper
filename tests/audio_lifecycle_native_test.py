"""Original game-entry/exit audio slices and exact Sound command branch."""
import pathlib,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,n):u.mem_write(a,struct.pack('<I',n&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
events=[]
def hook(uc,a,size,user):
 if a not in [0x530000,0x530100,0x40a5b0,0x40a8c0,0x40aa80,0x416620,0x416170,0x40a650,0x404090]:return
 sp=uc.reg_read(UC_X86_REG_ESP);clean=0
 if a==0x530000:events.append(('capture',get(sp+4)));clean=4
 elif a==0x530100:events.append(('release_capture',))
 elif a==0x40a5b0:events.append(('open',get(sp+4),get(sp+8)))
 elif a==0x40a8c0:events.append(('stop_cache',))
 elif a==0x404090:events.append(('receive',get(sp+4),get(sp+8),get(sp+12)))
 elif a==0x40aa80:events.append(('release_voice',uc.reg_read(UC_X86_REG_ECX)))
 elif a in [0x416620,0x416170]:events.append(('free',get(sp+4)))
 else:events.append(('close_device',))
 uc.reg_write(UC_X86_REG_EAX,0);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
for i in range(800):
 preference=[0,1,2,0xffffffff][i%4];voice=0x570000 if i%3 else 0;one=0x580000 if i%5 else 0;two=0x590000 if i%7 else 0
 for a,v in [(0x42e53c,preference),(0x422240,87),(0x435570,0x530000),(0x43556c,0x530100),(0x4281c4,voice),(0x428320,one),(0x428328,two)]:put(a,v)
 events.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4026c0,0x402709,count=1000)
 assert events==[('capture',87)]+([('open',87,0x20000)] if preference else [])
 events.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x403880,0x4038da,count=1000)
 expected=[('release_capture',),('stop_cache',)]+([('release_voice',voice),('free',voice)] if voice else [])+[('free',p) for p in [one,two] if p]+([('close_device',)] if preference else [])
 assert events==expected,(i,events,expected)
 events.clear();put(0x42deb0,19);put(0x42debc,23);u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x40c68b,0x40cddb,count=100)
 assert get(0x42e53c)==int(preference==0) and get(0x42deb0)==19 and get(0x42debc)==23 and not events
print('PASS2400 native lifecycle traces:conditional per-game device open,exit cache/dynamic/free/close order,Sound preference toggle without live mute/reinit')

for case in range(400):
 data=0x560000 if case%3 else 0
 for a,v in [(0x800000,0x540000),(0x800004,case),(0x800008,case*7),(0x80000c,data),(0x800010,case*11)]:put(a,v)
 events.clear();u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x4115e0,0x540000,count=100)
 assert events==([('receive',0,data,case*11)] if data else [])
print('PASS400 offline voice-forward cases:recipient/slot ignored,sender0,NULL ignored,length preserved')
