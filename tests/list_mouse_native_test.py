"""Mouse repeat timer and complete statistics hit/drag branch against x86."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Mouse(C.Structure):_fields_=[(n,C.c_int32) for n in ['x','y','held','event','repeat','drag','present','width','height','scroll','count','selected','history','dirty','screen_height']]
lib=C.CDLL(str(root/'code/build/liblist_mouse.so'))
lib.sniper_list_mouse.argtypes=[C.POINTER(Mouse)];lib.sniper_mouse_repeat.argtypes=[C.POINTER(C.c_int32),C.c_int32,C.c_int32,C.c_int32]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
r=random.Random(87);dragged=selected=0
for case in range(3600):
    timer=C.c_int32(r.choice([-2147483648,-1,0,1,8,12,2147483647]));ticks=r.choice([0,1,8,12,13,50,2147483647]);left=case%3==0;right=case%7==0
    for a,v in [(0x429f54,timer.value),(0x4218c8,ticks),(0x4222a4,left),(0x4222a8,right)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x405220,0x40525a,count=100)
    repeat=lib.sniper_mouse_repeat(C.byref(timer),ticks,left,right)
    assert timer.value==get(0x429f54) and repeat==u.reg_read(UC_X86_REG_EBX),case
    height=r.choice([26,34,50,82]);width=r.choice([50,110,230])
    m=Mouse(r.choice([-5,0,20,21,39,40,56,57,width,width+1]),r.choice([0,8,9,22,23,24,31,height+5,height+6,height+7,height+10,height+11,200]),case%5!=0,left,repeat,case%4==0,case%6!=0,width,height,r.randrange(4),12,r.randrange(12),r.randrange(1,10),0,r.choice([24,240,480,768]))
    old_drag=m.drag;old_selection=m.selected
    for a,v in [(0x42229c,m.x),(0x4222a0,m.y),(0x4222a4,m.event),(0x42dd70,0x520000 if m.present else 0),(0x520888,m.width),(0x52088c,m.height),(0x4283e4,m.drag),(0x4282f0,m.scroll),(0x429328,m.count),(0x429f58,m.selected),(0x42e538,m.history),(0x4289f4,m.dirty),(0x42434c,m.screen_height)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);put(0x800014,m.held);u.reg_write(UC_X86_REG_EBX,m.repeat)
    u.emu_start(0x405d56,0x405eca,count=1000);lib.sniper_list_mouse(C.byref(m))
    assert [m.drag,m.selected,m.history,m.dirty]==[get(a) for a in [0x4283e4,0x429f58,0x42e538,0x4289f4]],case
    dragged+=m.drag!=old_drag;selected+=m.selected!=old_selection
assert dragged>0 and selected>0
print(f'PASS3600 native mouse/timer cases: header repeats,row selection,border dragging,release; {dragged} drag transitions,{selected} selection changes')
