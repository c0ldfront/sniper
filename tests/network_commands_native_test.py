"""Native score wire parser and squad aggregation slices, original stricmp.
Only realloc intercepted; parser tails zeroed for unspecified allocation bytes.
"""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_EBP,UC_X86_REG_EBX
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/platform/network_commands.c'),str(root/'code/platform/score_dialog.c'),'-o',str(root/'code/build/libnetwork_commands.so')],check=True)
class Player(C.Structure):_fields_=[('name',C.c_char*32),('squad',C.c_char*32),('wins',C.c_int16),('losses',C.c_int16),('points',C.c_int32)]
class Squad(C.Structure):_fields_=[('name',C.c_char*32),('wins',C.c_int16),('losses',C.c_int16),('points',C.c_int32),('members',C.c_int32)]
lib=C.CDLL(str(root/'code/build/libnetwork_commands.so'));lib.sniper87_network_score_parse.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(C.POINTER(Player)),C.POINTER(C.c_int)]
lib.sniper87_network_squad_scores.argtypes=[C.POINTER(Player),C.c_int,C.POINTER(C.POINTER(Squad)),C.POINTER(C.c_int)]
lib.sniper87_network_squad_packet.argtypes=[C.c_void_p,C.c_char_p,C.c_char_p]
crt=C.CDLL(None);crt.free.argtypes=[C.c_void_p]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def put(a,n):u.mem_write(a,struct.pack('<I',n))
size=0

def hook(uc,a,sz,_):
 global size
 if a==0x416280:
  sp=uc.reg_read(UC_X86_REG_ESP);dst=get(sp+4);u.mem_write(dst,bytes(u.mem_read(get(sp+8),get(sp+12))));uc.reg_write(UC_X86_REG_EAX,dst);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4);return
 if a!=0x416210:return
 sp=uc.reg_read(UC_X86_REG_ESP);p=get(sp+4);n=get(sp+8);result=p or 0x650000
 if n>size:u.mem_write(result+size,bytes(n-size))
 size=n;uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(500):
 count=r.randrange(1,25);players=(Player*count)()
 for i,p in enumerate(players):p.name=f'player{i}'.encode();p.squad=r.choice([b'',b'A',b'a',b'B',b'b',b'C',b'unique'+str(i).encode()]);p.wins=r.randrange(-32768,32768);p.losses=r.randrange(-32768,32768);p.points=r.randrange(-2147483648,2147483648)
 wire=b'\x0b'+b''.join(p.name+b'\0'+p.squad+b'\0'+struct.pack('<hhi',p.wins,p.losses,p.points) for p in players)
 out=C.POINTER(Player)();n=C.c_int();assert lib.sniper87_network_score_parse(wire,len(wire),C.byref(out),C.byref(n));assert n.value==count
 size=0;put(0x422288,0);put(0x422290,0);u.mem_write(0x600000,wire[1:]);put(0x800000-16,0x600000+len(wire)-1);u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000);u.reg_write(UC_X86_REG_EBX,0x600000)
 u.emu_start(0x40bedf,0x40bfeb,count=100000);assert get(0x422290)==count;assert C.string_at(out,count*72)==bytes(u.mem_read(get(0x422288),count*72));crt.free(out)
 arr=(Player*count)(*players);squads=C.POINTER(Squad)();ns=C.c_int();assert lib.sniper87_network_squad_scores(arr,count,C.byref(squads),C.byref(ns))
 size=0;u.mem_write(0x620000,bytes(players));put(0x422288,0x620000);put(0x422290,count);u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000)
 u.emu_start(0x40c038,0x40c293,count=1000000);assert bytes(arr)==bytes(u.mem_read(0x620000,count*72));assert get(0x422294)==ns.value
 if ns.value:assert C.string_at(squads,ns.value*44)==bytes(u.mem_read(get(0x42228c),ns.value*44)),case
 crt.free(squads)
for bad in [b'',b'\x0babc',b'\x0ba\0b\0\0',b'\x0b'+b'a'*32+b'\0\0'+bytes(8)]:
 out=C.POINTER(Player)();n=C.c_int();assert not lib.sniper87_network_score_parse(bad,len(bad),C.byref(out),C.byref(n))
p=C.create_string_buffer(65);assert lib.sniper87_network_squad_packet(p,b'  Team  ',b' pass ');assert p.raw==b'\x13Team'+bytes(28)+b' pass '+bytes(26)
assert not lib.sniper87_network_squad_packet(p,b'   ',b'password')
for i in range(250):
 name=r.choice([b'',b' ',b'  Team  ',b' X',b'Z '*15,b'Normal',b' '*31]);password=r.choice([b'',b' pass ',b'p'*31]);u.mem_write(0x42ded0,name+b'\0');u.mem_write(0x42e120,password+b'\0');u.mem_write(0x7fff00,bytes(256));u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000)
 u.emu_start(0x40c706,0x40c7da,count=10000) if name.strip() else u.emu_start(0x40c706,0x40cddb,count=10000)
 p=C.create_string_buffer(65);ok=lib.sniper87_network_squad_packet(p,name,password);assert bool(ok)==bool(name.strip())
 if ok:assert p.raw==bytes(u.mem_read(0x800000-0x4d,65)),i
print('PASS500 native variable-string score parsers and500 squad aggregations: singleton exclusion, empty squads, case-insensitive sorting, wrapped16/32 sums; 250 native squad trim/packet stages; malformed bounds')
