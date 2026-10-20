"""Compare full406ea0 commands, with sprite methods and DirectDraw intercepted.
No pixel-level claims; intentionally includes unsafe offscreen native spans.
"""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
fields=['cursor_x','cursor_y','camera_x','camera_y','screen_width','screen_height','selected_tile','selection_active','x1','y1','x2','y2','clipboard_active','clipboard_width','clipboard_height','selection_color','clipboard_color','palette_background_color']
addresses=[0x4218d4,0x4218d8,0x42de80,0x424324,0x42818c,0x42434c,0x4218dc,0x429324,0x4281bc,0x428a6c,0x4283dc,0x429f4c,0x4281ac,0x428a04,0x4281a0,0x424328,0x424350,0x429278]
class State(C.Structure):_fields_=[(n,C.c_int32) for n in fields]
class Command(C.Structure):_fields_=[(n,C.c_int32) for n in ['kind','x','y','length','right','bottom','color']]
Draw=C.CFUNCTYPE(C.c_int,C.c_void_p,C.POINTER(Command))
lib=C.CDLL(str(root/'code/build/libeditor_render.so'));lib.sniper_editor_render.argtypes=[C.POINTER(State),Draw,C.c_void_p]
ce=[];ne=[];lock_ok=True
@Draw
def draw(_,p):
 c=p.contents;ce.append(tuple(getattr(c,n) for n,_ in Command._fields_));return int(lock_ok)
def hook(uc,a,size,user):
 if a not in [0x4105c0,0x4106f0,0x415f80,0x415c80,0x415cb0,0x600000,0x600010]:return
 sp=uc.reg_read(UC_X86_REG_ESP);pop=0;result=0
 if a==0x4105c0:
  assert [get(sp+i) for i in [12,16,20]]==[0,0,0]
  ne.append((1,get(sp+4),get(sp+8),0,0,0,0));pop=20
 elif a==0x4106f0:
  assert get(sp+12)==1;ne.append((7,get(sp+4),get(sp+8),0,0,0,0));pop=12
 elif a==0x415f80:
  rect=get(sp+8);l,t,r,b=[get(rect+i*4) for i in range(4)]
  ne.append((6,l,t,0,r,b,get(sp+12)))
 elif a in [0x415c80,0x415cb0]:ne.append((4 if a==0x415c80 else 3,get(sp+12),get(sp+16),get(sp+20),0,0,get(sp+24)))
 elif a==0x600000:
  assert get(sp+8)==0 and get(sp+16)==1 and get(sp+20)==0
  desc=get(sp+12);assert get(desc)==108;put(desc+16,1024);put(desc+36,0x650000)
  ne.append((2,0,0,0,0,0,0));result=0 if lock_ok else -1;pop=20
 elif a==0x600010:ne.append((5,0,0,0,0,0,0));pop=8
 uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
u.hook_add(UC_HOOK_CODE,hook)
put(0x42931c,0x610000);put(0x610000,0x611000);put(0x611064,0x600000);put(0x611080,0x600010)
r=random.Random(40687);total=0
for case in range(2400):
 s=State();s.screen_width=r.choice([320,640,800,1024,1280]);s.screen_height=r.choice([200,240,480,600,768]);s.cursor_x=r.randrange(1024);s.cursor_y=r.randrange(1024)
 s.camera_x=s.cursor_x*16-r.randrange(-50,s.screen_width+50);s.camera_y=s.cursor_y*16-r.randrange(-50,s.screen_height+50)
 s.selected_tile=r.randrange(1,153);s.selection_active=r.randrange(2);s.clipboard_active=r.randrange(2)
 s.x1=s.cursor_x+r.randrange(-40,40);s.x2=s.cursor_x+r.randrange(-40,40);s.y1=s.cursor_y+r.randrange(-30,30);s.y2=s.cursor_y+r.randrange(-30,30)
 s.clipboard_width=r.randrange(1,80);s.clipboard_height=r.randrange(1,80)
 s.selection_color=r.choice([-1,0,1,8,255]);s.clipboard_color=r.choice([-1,0,9,63,128]);s.palette_background_color=r.randrange(256);lock_ok=r.randrange(4)!=0
 if case%5==0:
  # Arithmetic extremes verify x86 wrapping and signed division outside UI ranges.
  for name in r.sample(fields[:15],5):setattr(s,name,r.choice([-2147483648,2147483647,-1,0,1]))
 for field,address in zip(fields,addresses):put(address,getattr(s,field))
 ce.clear();ne.clear();put(0x800000,0x700000);u.reg_write(UC_X86_REG_ESP,0x800000)
 u.emu_start(0x406ea0,0x700000,count=10000)
 lib.sniper_editor_render(C.byref(s),draw,None)
 assert ce==ne,(case,dict((n,getattr(s,n)) for n in fields),ce,ne)
 assert all(get(a)==getattr(s,n) for n,a in zip(fields,addresses))
 total+=len(ne)
print(f'PASS2400 original editor-render traces: {total} matching commands; selection/paste priority, clipping, lock failure, palette/cursors, signed/wrapped coordinates; no input mutation')
