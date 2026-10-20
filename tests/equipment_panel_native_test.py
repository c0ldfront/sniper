"""Equipment cache original408158..4086ef;shared COM fixture with team panel."""
import hud_panel_native_test as base
from hud_panel_native_test import *
from unicorn.x86_const import UC_X86_REG_ESI
u.hook_del(hook)
Rect=C.CFUNCTYPE(None,C.c_void_p,Handle,*([C.c_int32]*4),C.c_uint32)
class Equipment(C.Structure):_fields_=[('cache',Panel),('height',C.c_int32),('highlight',C.c_uint32),('rect',Rect),('transparent',Text)]
@Create
def create_equipment(_,w,h):actual.append(('create',w,h));return NEW
@Rect
def rect(_,s,l,t,r,b,c):actual.append(('rect',s,l,t,r,b,c))
@Text
def transparent(_,s,t,x,y,c):actual.append(('transparent',s,t.decode(),x,y,c))
ops2=Ops(release,create_equipment,key,clear,text,blit,None)
lib.sniper_equipment_panel_draw.argtypes=[C.POINTER(Equipment)]
def equipment_intercept(uc,a,size,user):
    if a not in {CREATE,0x416bc0,0x4107d0,0x415cf0,0x415e50}:return base.intercept(uc,a,size,user)
    sp=uc.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0];result=0;pop=0
    if a==CREATE:
        obj,desc,out,outer=words(sp+4,4);assert obj==DD and outer==0
        raw=list(words(desc,27));assert raw[:5]==[108,7,64,60,0] and raw[26]==64
        raw[0]=raw[1]=raw[2]=raw[3]=raw[26]=0;assert not any(raw)
        word(out,NEW);native.append(('create',60,64));pop=4
    elif a==0x416bc0:
        dest,fmt=words(sp+4,2);f=string(fmt);values=tuple(map(signed,words(sp+12,f.count('%'))));t=f%values
        u.mem_write(dest,t.encode()+b'\0');result=len(t)
    elif a==0x415cf0:
        obj,ptr,x,y,c=words(sp+4,5);native.append(('transparent',obj,string(ptr),signed(x),signed(y),c))
    elif a==0x415e50:
        obj,ptr,color=words(sp+4,3);native.append(('rect',obj,*map(signed,words(ptr,4)),color))
    else:
        obj,dst,src,srect,flags=words(sp+4,5);assert obj==0 and flags==0x8000
        l,t,r,b=map(signed,words(dst,4));assert words(srect,4)==(0,0,60,64)
        native.append(('blit',src,l,t,signed(r-l),signed(b-t)))
    uc.reg_write(UC_X86_REG_ESP,sp+4+4*pop);uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,equipment_intercept)
rng=random.Random(87);reuse=0;highlights=0
fields=[0xf0,0xf4,0xd4,0x90,0xf8,0xd8,0x94,0xfc,0xec,0x98,0xdc,0x88,0x100,0xd0,0x8c]
for case in range(1200):
    ship=Ship()
    for off in fields:ship.word[off//4]=rng.choice([0,0,1,-1,3,2147483647,-2147483648])
    if case==0:
        for off in fields:ship.word[off//4]=0
    total=sum(ship.word[off//4] for off in fields)&0xffffffff
    panel=Panel(C.pointer(ship),OLD if case%3 else 0,total if case%2 else (total+1)&0xffffffff,rng.getrandbits(32),640,ops2)
    e=Equipment(panel,rng.choice([200,481,768,-17]),rng.getrandbits(32),rect,transparent)
    reuse+=bool(panel.surface and panel.cached_sum==total)
    u.mem_write(SHIP,bytes(ship))
    for a,v in [(0x4283e0,panel.surface),(0x429280,panel.cached_sum),(0x42815c,panel.background),(0x42434c,e.height),(0x429740,e.highlight)]:word(a,v)
    actual.clear();native.clear();u.reg_write(UC_X86_REG_ESP,STACK+32768);u.reg_write(UC_X86_REG_ESI,0)
    u.emu_start(0x408158,0x4086ef,count=100000)
    assert u.reg_read(UC_X86_REG_EIP)==0x4086ef
    assert lib.sniper_equipment_panel_draw(C.byref(e))==0
    assert actual==native,(case,actual,native)
    assert e.cache.surface==words(0x4283e0,1)[0] and e.cache.cached_sum==words(0x429280,1)[0]
    highlights+=sum(x[0]=='rect' for x in actual)
assert highlights>1000
print(f'1200 equipment panel traces/cache states match original x86; {reuse} cached draws and {highlights} highlights')
