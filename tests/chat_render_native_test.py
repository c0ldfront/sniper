"""Execute original408e70; intercept clock, DirectDraw, message sink, text draws.
Compare layout, history strings/colors/order, input positioning, ticker threshold,
and cache rebuild state. DirectDraw failure/recovery and raster pixels not covered.
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
def string(a):return bytes(u.mem_read(a,256)).split(b'\0')[0]
class Layout(C.Structure):_fields_=[(n,C.c_int32) for n in ['width','height','max_chars']]
class Rect(C.Structure):_fields_=[(n,C.c_int32) for n in ['left','top','right','bottom']]
Text=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_int32,C.c_int32,C.c_int32)
lib=C.CDLL(str(root/'code/build/libchat_render.so'))
lib.sniper_chat_layout.argtypes=[C.c_int32,C.c_int32,C.POINTER(Layout)]
lib.sniper_chat_history_draw.argtypes=[C.POINTER(Layout),C.c_void_p,C.POINTER(C.c_int32),C.c_int32,Text,C.c_void_p]
lib.sniper_chat_placement.argtypes=[C.c_int32,C.c_int32,C.c_int32,C.c_char_p,C.POINTER(Rect),Text,C.c_void_p]
lib.sniper_chat_ticker_due.argtypes=[C.c_uint32,C.c_uint32,C.c_int32,C.c_int32]
ce=[];ne=[];sizes=[];rects=[];clock_calls=0;ticker_calls=0;released=0
@Text
def text(_,value,x,y,color):ce.append((value,x,y,color))
def hook(uc,a,size,user):
    global clock_calls,ticker_calls,released
    if a not in [0x600000,0x600010,0x600020,0x600030,0x600040,0x600050,0x415e50,0x416090,0x415cf0,0x404c50]:return
    sp=uc.reg_read(UC_X86_REG_ESP);pop=0;result=0
    if a==0x600000:result=ms[clock_calls];clock_calls+=1
    elif a==0x600010:
        desc=get(sp+8);w=get(desc+12);h=get(desc+8);sizes.append((w,h));put(get(sp+12),0x610000);pop=16
    elif a==0x600020:released+=1;pop=4
    elif a==0x600030:pop=12
    elif a==0x600040:
        desc=get(sp+8);w,h=sizes[-1] if sizes else cached;put(desc+8,h);put(desc+12,w);pop=8
    elif a==0x600050:rects.append(tuple(get(get(sp+8)+i*4) for i in range(4)));assert get(sp+20)==0x8000;pop=24
    elif a in [0x416090,0x415cf0]:ne.append((string(get(sp+8)),get(sp+12),get(sp+16),get(sp+20)))
    elif a==0x404c50:ticker_calls+=1
    uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
u.hook_add(UC_HOOK_CODE,hook)
put(0x4353f4,0x600000)
put(0x620000,0x621000);put(0x621018,0x600010);put(0x4289f8,0x620000)
put(0x610000,0x611000)
for offset,addr in [(8,0x600020),(0x74,0x600030),(0x58,0x600040),(0x14,0x600050)]:put(0x611000+offset,addr)
put(0x42931c,0x610000);put(0x421a2c,1);put(0x421a30,0)
r=random.Random(87)
for case in range(1200):
    rows=r.randrange(0,17);width=r.choice([320,640,800,1024]);height=r.choice([200,240,480,600,768]);dirty=r.randrange(2);exists=r.randrange(2)
    ms=[r.randrange(2**32),r.randrange(2**32)];last=r.randrange(2**32);interval=r.choice([-1,0,100,300,2**31-1]);enabled=r.randrange(2)
    lines=[b'' if r.randrange(3)==0 else bytes(r.randrange(32,127) for _ in range(r.randrange(1,128))) for i in range(rows)]
    colors=[r.randrange(4) for _ in range(rows)];input_text=b'' if r.randrange(2) else b'hello there'*r.randrange(1,8)
    cached=(r.choice([267,534]),8*r.randrange(3,17))
    ce.clear();ne.clear();sizes.clear();rects.clear();clock_calls=ticker_calls=released=0
    for address,value in [(0x42227c,rows),(0x42818c,width),(0x42434c,height),(0x428188,dirty),(0x428a60,0x610000 if exists else 0),(0x428a74,last),(0x4282e5,interval),(0x422284,enabled)]:put(address,value)
    for i,(line,color) in enumerate(zip(lines,colors)):u.mem_write(0x428a78+i*128,line+b'\0');put(0x428a10+i*4,color)
    u.mem_write(0x428358,input_text+b'\0');put(0x800000,0x700000);u.reg_write(UC_X86_REG_ESP,0x800000)
    u.emu_start(0x408e70,0x700000,count=1000000)
    layout=Layout();valid=lib.sniper_chat_layout(width,rows,C.byref(layout))
    if valid:
        rebuilt=dirty or not exists
        if rebuilt:
            assert sizes==[(layout.width,layout.height)],case
            data=b''.join(v.ljust(128,b'\0') for v in lines)
            lib.sniper_chat_history_draw(C.byref(layout),C.c_char_p(data),(C.c_int32*rows)(*colors),rows,text,None)
            assert get(0x428188)==0 and (get(0x428a74)&0xffffffff)==ms[1]//10
            assert released==exists
        else:assert sizes==[]
        w,h=(layout.width,layout.height) if rebuilt else cached
        rect=Rect();lib.sniper_chat_placement(height,w,h,input_text,C.byref(rect),text,None)
        assert rects==[(rect.left,rect.top,rect.right,rect.bottom)],(case,rects)
        assert ticker_calls==lib.sniper_chat_ticker_due(ms[0],last,interval,enabled)
        assert clock_calls==1+bool(rebuilt)
    else:assert not sizes and not rects and not clock_calls and not ticker_calls
    assert ce==ne,(case,ce,ne)
print('PASS1200 original408e70 traces: layout, truncation, empty rows, colors, input/history position, cache rebuild, signed ticker thresholds; controlled successful DirectDraw')
