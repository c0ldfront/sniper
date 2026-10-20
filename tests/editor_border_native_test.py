"""Execute415f80 pixel writes, proving inclusive OUTLINE semantics.
Existing editor_render_native_test intercepts this callee; this closes that gap.
Full recreated/native screenshot comparison is separate editor_visual_parity_test.
"""
from pathlib import Path
import random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
put(0x500000,0x500100);put(0x500164,0x530000);put(0x500180,0x530010)
r=random.Random(41580);pitch=648;pixels=bytes(r.randrange(256) for _ in range(pitch*480));locked=False;ok=True

def hook(uc,a,n,data):
 global locked
 if a not in [0x530000,0x530010]:return
 sp=uc.reg_read(UC_X86_REG_ESP)
 if a==0x530000:
  assert not locked and get(sp+4)==0x500000 and get(sp+8)==0 and get(sp+16)==1 and get(sp+20)==0
  d=get(sp+12);assert get(d)==108;put(d+16,pitch);put(d+36,0x600000);locked=ok;pop=20
  uc.reg_write(UC_X86_REG_EAX,0 if ok else 0x887601c2)
 else:
  assert locked and get(sp+4)==0x500000 and get(sp+8)==0x600000;locked=False;pop=8;uc.reg_write(UC_X86_REG_EAX,0)
 uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
u.hook_add(UC_HOOK_CODE,hook)
for case in range(500):
 l=r.randrange(640);t=r.randrange(480);rr=r.randrange(l,640);b=r.randrange(t,480);color=r.getrandbits(32);ok=case%11!=0
 if case==0:l,t,rr,b,color,ok=167,7,472,136,255,True
 u.mem_write(0x600000,pixels)
 for j,v in enumerate([l,t,rr,b]):put(0x510000+4*j,v)
 for j,v in enumerate([0x530100,0x500000,0x510000,color]):put(0x800000+4*j,v)
 u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x415f80,0x530100,count=1000000)
 expected=bytearray(pixels)
 if ok:
  for x in range(l,rr+1):expected[t*pitch+x]=expected[b*pitch+x]=color&255
  for y in range(t,b):expected[y*pitch+l]=expected[y*pitch+rr]=color&255
 assert not locked and bytes(u.mem_read(0x600000,len(pixels)))==bytes(expected),case
print('PASS500 actual415f80 pixel buffers: inclusive outline,untouched interior/padding,degenerate rectangles,failed lock')
