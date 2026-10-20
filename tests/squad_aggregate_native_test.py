#!/usr/bin/env python3
"""Execute40c038..40c293 including actual416930 qsort and40d940 comparator.
Only realloc is intercepted (zeroing unused name-tail allocation bytes)."""
import ctypes as C,pathlib,random,struct,subprocess
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_EBP
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/platform/score_dialog.c'),'-o',str(root/'code/build/libscore_dialog.so')],check=True)
class Player(C.Structure):_fields_=[('name',C.c_char*32),('squad',C.c_char*32),('wins',C.c_int16),('losses',C.c_int16),('points',C.c_int32)]
class Squad(C.Structure):_fields_=[('name',C.c_char*32),('wins',C.c_int16),('losses',C.c_int16),('points',C.c_int32),('members',C.c_int32)]
lib=C.CDLL(str(root/'code/build/libscore_dialog.so'));f=lib.sniper87_build_squad_scores;f.argtypes=[C.POINTER(Player),C.c_int,C.POINTER(Squad),C.c_int]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def put(a,n):u.mem_write(a,struct.pack('<I',n))
size=0

def hook(uc,a,sz,_):
 global size
 if a!=0x416210:return
 sp=uc.reg_read(UC_X86_REG_ESP);p=get(sp+4);n=get(sp+8);result=p or 0x650000
 if n>size:u.mem_write(result+size,bytes(n-size))
 size=n;uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook);put(0x423f98,0)
r=random.Random(9387)
for case in range(1000):
 count=case if case<16 else r.randrange(1,100);players=(Player*count)()
 for i,p in enumerate(players):
  p.name=f'player{i}'.encode();p.squad=r.choice([b'',b'A',b'a',b'B',b'b',b'C',b'unique'+str(i).encode(),b'Long squad name'])
  if case%7==0:p.squad=[b'ALPHA',b'alpha',b'aLpHa'][i%3]
  p.wins=r.randrange(-32768,32768);p.losses=r.randrange(-32768,32768);p.points=r.randrange(-2147483648,2147483648)
 size=0
 if count:u.mem_write(0x620000,bytes(players))
 put(0x422288,0x620000);put(0x422290,count);u.reg_write(UC_X86_REG_EBP,0x800000);u.reg_write(UC_X86_REG_ESP,0x7ff000)
 u.emu_start(0x40c038,0x40c293,count=3000000)
 output=(Squad*(count//2))();n=f(players,count,output,count//2)
 assert n==get(0x422294),(case,n,get(0x422294))
 assert bytes(players)==bytes(u.mem_read(0x620000,count*72)),('tie permutation',case)
 if n:assert bytes(output)[:n*44]==bytes(u.mem_read(get(0x42228c),n*44)),('aggregate',case)
assert f(None,-1,None,0)==-1
p=(Player*2)();p[0].squad=p[1].squad=b'A';assert f(p,2,None,0)==-1
print('PASS1000 full original sort+aggregate cases: exact unstable CRT tie permutations,empty/singleton exclusion,case-retained names,16/32-bit wrapped sums; shortsort/quicksort and all-equal runs')
