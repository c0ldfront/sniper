"""Full404c50 with original CRT memmove and sprintf, compares all buffer bytes."""
import ctypes as C,pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
lib=C.CDLL(str(root/'code/build/libchat_history.so'))
Row=C.c_char*128;Rows=Row*16;Colors=C.c_int32*16
lib.sniper_chat_history.argtypes=[C.POINTER(Row),C.POINTER(C.c_int32),C.c_int32,C.c_int32,C.POINTER(C.c_int32),C.c_char_p,C.c_char_p,C.c_int32]
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
r=random.Random(40450)
for case in range(1500):
    lines=Rows();colors=Colors();C.memset(C.addressof(lines),0x5a,C.sizeof(lines))
    for i in range(16):lines[i].value=f'old line{i}'.encode();colors[i]=r.randrange(-20,20)
    count=r.choice([-2,0,1,2,3,4,8,16]);width=r.choice([320,640,800,319]);dirty=C.c_int32(r.randrange(-1,2))
    length=r.choice([0,1,31,32,33,42,43,44,74,75,76,99,100,101,255]);text=bytes(r.choice(b'XYZ abcdef0123') for _ in range(length))
    name=r.choice([None,b'',b'A',b'1234567890',b'1234567890123456789012345678901']);recipient=r.choice([-2,-1,0,1,1023]);sender=r.choice([-1,0,2])
    effective=name if sender>=0 else None
    u.mem_write(0x530000,text+b'\0');u.mem_write(0x500000,(name or b'')+b'\0')
    for a,v in [(0x42227c,count),(0x42818c,width),(0x428188,dirty.value),(0x429338,0x500000 if name is not None else 0),(0x429340,0x500000 if name is not None else 0),(0x800000,0x800100),(0x800004,0x530000),(0x800008,sender),(0x80000c,recipient)]:put(a,v)
    u.mem_write(0x428a78,bytes(lines));u.mem_write(0x428a10,bytes(colors))
    u.reg_write(UC_X86_REG_ESP,0x800000);u.emu_start(0x404c50,0x800100,count=100000)
    lib.sniper_chat_history(lines,colors,count,width,C.byref(dirty),text,effective,recipient)
    assert bytes(lines)==bytes(u.mem_read(0x428a78,C.sizeof(lines))),(case,count,width,length,effective)
    assert bytes(colors)==bytes(u.mem_read(0x428a10,C.sizeof(colors))),(case,'colors')
    assert dirty.value==get(0x428188),(case,'dirty')
print('PASS1500 full native chat receive cases, original CRT formatting/memmove: all128-byte slots/colors, split boundaries, sender naming, routes and suppressed short history')
