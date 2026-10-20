"""Original text-panel geometry and drawing trace; surface APIs are controlled."""
import ctypes as C
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,128)).split(b'\0')[0]
class Row(C.Structure):_fields_=[('text',C.c_char_p),('color',C.c_int32)]
class Layout(C.Structure):_fields_=[(n,C.c_int32) for n in ['width','height','columns','stride']]
Line=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int32,C.c_int32,C.c_int32)
Text=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32,C.c_int32)
class Draw(C.Structure):_fields_=[('line',Line),('text',Text),('user',C.c_void_p)]
ce=[];ne=[]
@Line
def line(_,vertical,x,y,length):ce.append(('line',vertical,x,y,length))
@Text
def text(_,value,x,y,color):ce.append(('text',value,x,y,color))
draw=Draw(line,text,None)
lib=C.CDLL(str(root/'code/build/libtext_panel.so'))
lib.sniper_text_panel_layout.argtypes=[C.c_char_p,C.POINTER(Row),C.c_int32,C.c_int32,C.POINTER(Layout)]
lib.sniper_text_panel_draw.argtypes=[C.c_char_p,C.POINTER(Row),C.c_int32,C.c_int32,C.POINTER(Layout),C.POINTER(Draw)]
def hook(uc,a,size,user):
    if a==0x4128ac:uc.reg_write(UC_X86_REG_EIP,0x4128c0);return # unlock omitted
    if a not in [0x415cb0,0x415c80,0x416090]:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x416090:ne.append(('text',string(get(sp+8)),get(sp+12),get(sp+16),get(sp+20)))
    else:ne.append(('line',int(a==0x415c80),get(sp+12),get(sp+16),get(sp+20)))
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(500):
    count=r.choice([1,2,9,25,50,51,100,255,256]);per=r.choice([1,2,9,25,50,256])
    title=b'' if case%3==0 else b'Help v.87'+b'!'*(case%80)
    values=[(b'x'*r.randrange(0,101),r.randrange(4)) for i in range(count)]
    rows=(Row*count)(*[Row(t,c) for t,c in values]);layout=Layout();ce.clear();ne.clear()
    assert lib.sniper_text_panel_layout(title,rows,count,per,C.byref(layout))==1
    lib.sniper_text_panel_draw(title,rows,count,per,C.byref(layout),C.byref(draw))
    u.mem_write(0x500000,b'\0'*0x894);u.mem_write(0x500000,title+b'\0')
    put(0x500880,count);put(0x500890,per)
    for i,(value,color) in enumerate(values):
        put(0x500080+i*4,0x600000+i*128);put(0x500480+i*4,color);u.mem_write(0x600000+i*128,value+b'\0')
    u.reg_write(UC_X86_REG_ECX,0x500000);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x412610,0x4126d7,count=1000000)
    assert (layout.width,layout.height)==(get(0x500888),get(0x50088c)),case
    sp=u.reg_read(UC_X86_REG_ESP)
    for offset,value in [(0x50,layout.height),(0x54,layout.width),(0x58,layout.width),(0x6c,0x700000)]:put(sp+offset,value)
    put(0x500884,0x520000)
    u.emu_start(0x4127be,0x41294d,count=1000000)
    assert ce==ne,(case,next(((a,b) for a,b in zip(ce,ne) if a!=b),('length',len(ce),len(ne))))
print('PASS500 original text-panel layout/draw traces: widths, heights, borders, separators, titles, columns, text coordinates/colors; actual native font widths')
