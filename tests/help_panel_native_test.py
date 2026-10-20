"""Help content/constructor/display arguments and F1 toggle against original."""
import ctypes as C
import pathlib,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,4096);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
class Row(C.Structure):_fields_=[('text',C.c_char_p),('color',C.c_int32)]
lib=C.CDLL(str(root/'code/build/libhelp_panel.so'));lib.sniper_help_rows.argtypes=[C.c_int,C.POINTER(C.c_int32)];lib.sniper_help_rows.restype=C.POINTER(Row)
lib.sniper_help_toggle.argtypes=[C.c_int32,C.c_int32]
def put(a,x):u.mem_write(a,struct.pack('<I',x&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,128)).split(b'\0')[0]
rows=[];events=[]
def hook(uc,a,size,user):
    if a not in [0x416bc0,0x416b20,0x412560,0x412a20,0x412610]:return
    sp=uc.reg_read(UC_X86_REG_ESP);clean=0
    if a==0x416bc0:
        assert string(get(sp+8))==b'Help v.%d' and get(sp+12)==87
        u.mem_write(get(sp+4),b'Help v.87\0')
    elif a==0x416b20:
        assert get(sp+4)==0x894;uc.reg_write(UC_X86_REG_EAX,0x500000)
    elif a==0x412560:
        events.append(('create',string(get(sp+4)),get(sp+8)));uc.reg_write(UC_X86_REG_EAX,0x500000);clean=8
    elif a==0x412a20:rows.append((string(get(sp+4)),get(sp+8)));clean=8
    else:
        events.append(('draw',get(sp+4),get(sp+8)));uc.emu_stop();return
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
for spectator,address in [(0,0x408700),(1,0x408970)]:
    count=C.c_int32();data=lib.sniper_help_rows(spectator,C.byref(count));expected=[(data[i].text,data[i].color) for i in range(count.value)]
    for cached in [0,1]:
        rows.clear();events.clear();put(0x428350,0x500000 if cached else 0);put(0,0)
        u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(address,0x408ab0,count=100000)
        assert rows==([] if cached else expected),(spectator,rows,expected)
        assert events==([('draw',8,8)] if cached else [('create',b'Help v.87',50),('draw',8,8)]),events
for mode in [-1,0,1,2,3,4,5,2147483647]:
    for ship_type in [-1,0,1,3,4,5]:
        put(0x4282ec,mode);put(0x4218e0,0x500000);put(0x500040,ship_type)
        u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x405887,0x405d56,count=1000)
        assert lib.sniper_help_toggle(mode,ship_type)==get(0x4282ec)
print('PASS original flight/spectator help rows, title, capacity, placement, cached path and48 F1 mode transitions')
