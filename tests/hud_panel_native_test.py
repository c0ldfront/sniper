"""Team/bounty cache407f9c..408158: surface lifecycle and draw traces."""
import ctypes as C
import pathlib,struct,random
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
STACK,SHIP,DD,OLD,NEW,VT,RELEASE,CREATE,KEY=[0x1000000+i*65536 for i in range(9)]
for a in (STACK,SHIP,DD,OLD,NEW,VT,RELEASE,CREATE,KEY):u.mem_map(a,65536)
def word(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def words(a,n):return struct.unpack('<'+'I'*n,u.mem_read(a,n*4))
def signed(v):return C.c_int32(v).value
def string(a):
    b=bytearray()
    while u.mem_read(a,1)[0]:b+=u.mem_read(a,1);a+=1
    return b.decode()
for a in (DD,OLD,NEW):word(a,VT)
for offset,fn in ((8,RELEASE),(0x18,CREATE),(0x74,KEY)):word(VT+offset,fn)
word(0x4218e0,SHIP);word(0x4289f8,DD)
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
Handle=C.c_size_t
Release=C.CFUNCTYPE(None,C.c_void_p,Handle)
Create=C.CFUNCTYPE(Handle,C.c_void_p,C.c_int32,C.c_int32)
Key=C.CFUNCTYPE(None,C.c_void_p,Handle,C.c_uint32)
Text=C.CFUNCTYPE(None,C.c_void_p,Handle,C.c_char_p,C.c_int32,C.c_int32,C.c_int)
Blit=C.CFUNCTYPE(None,C.c_void_p,Handle,*([C.c_int32]*4))
class Ops(C.Structure):_fields_=[('release',Release),('create',Create),('key',Key),('clear',Key),('text',Text),('blit',Blit),('user',C.c_void_p)]
class Panel(C.Structure):_fields_=[('local',C.POINTER(Ship)),('surface',Handle),('cached_sum',C.c_uint32),('background',C.c_uint32),('width',C.c_int32),('ops',Ops)]
actual=[];native=[]
@Release
def release(_,s):actual.append(('release',s))
@Create
def create(_,w,h):actual.append(('create',w,h));return NEW
@Key
def key(_,s,c):actual.append(('key',s,c))
@Key
def clear(_,s,c):actual.append(('clear',s,c))
@Text
def text(_,s,t,x,y,c):actual.append(('text',s,t.decode(),x,y,c))
@Blit
def blit(_,s,x,y,w,h):actual.append(('blit',s,x,y,w,h))
ops=Ops(release,create,key,clear,text,blit,None)
lib=C.CDLL(str(ROOT/'code/build/libhud_panel.so'));lib.sniper_team_panel_draw.argtypes=[C.POINTER(Panel)]
hooks={RELEASE,CREATE,KEY,0x416bc0,0x415dd0,0x415df0,0x416090,0x4107d0}
def intercept(uc,a,size,user):
    if a not in hooks:return
    sp=uc.reg_read(UC_X86_REG_ESP);ret=words(sp,1)[0];pop=0;result=0
    if a==RELEASE:native.append(('release',words(sp+4,1)[0]));pop=1
    elif a==CREATE:
        obj,desc,out,outer=words(sp+4,4);assert obj==DD and outer==0
        raw=list(words(desc,27));assert raw[:5]==[108,7,16,48,0] and raw[26]==64
        raw[0]=raw[1]=raw[2]=raw[3]=raw[26]=0;assert not any(raw)
        word(out,NEW);native.append(('create',48,16));pop=4
    elif a==KEY:
        obj,flags,ptr=words(sp+4,3);lo,hi=words(ptr,2);assert flags==8 and lo==hi
        native.append(('key',obj,lo));pop=3
    elif a==0x416bc0:
        dest,fmt,value=words(sp+4,3);t=string(fmt)%signed(value);u.mem_write(dest,t.encode()+b'\0');result=len(t)
    elif a==0x415dd0:result=len(string(words(sp+4,1)[0]))*6
    elif a==0x415df0:native.append(('clear',*words(sp+4,2)))
    elif a==0x416090:
        obj,ptr,x,y,c=words(sp+4,5);native.append(('text',obj,string(ptr),signed(x),signed(y),c))
    else:
        obj,dst,src,srect,flags=words(sp+4,5);assert obj==0 and flags==0x8000
        l,t,r,b=map(signed,words(dst,4));assert words(srect,4)==(0,0,48,16)
        native.append(('blit',src,l,t,signed(r-l),signed(b-t)))
    uc.reg_write(UC_X86_REG_ESP,sp+4+4*pop);uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,ret)
hook=u.hook_add(UC_HOOK_CODE,intercept)
def test_team():
    rng=random.Random(87);reuse=0
    for case in range(900):
        ship=Ship();ship.word[0x68//4]=rng.choice([0,1,10,999,-1,2147483647,-2147483648]);ship.word[0xa0//4]=rng.randrange(-100,10000)
        total=(ship.word[0x68//4]+ship.word[0xa0//4])&0xffffffff
        panel=Panel(C.pointer(ship),OLD if case%3 else 0,total if case%2 else (total+1)&0xffffffff,rng.getrandbits(32),rng.choice([320,641,1024]),ops)
        reuse+=bool(panel.surface and panel.cached_sum==total)
        u.mem_write(SHIP,bytes(ship))
        for a,v in [(0x4281b0,panel.surface),(0x424344,panel.cached_sum),(0x42815c,panel.background),(0x42818c,panel.width)]:word(a,v)
        actual.clear();native.clear();u.reg_write(UC_X86_REG_ESP,STACK+32768)
        u.emu_start(0x407f9c,0x408158,count=100000)
        assert u.reg_read(UC_X86_REG_EIP)==0x408158
        assert lib.sniper_team_panel_draw(C.byref(panel))==0
        assert actual==native,(case,actual,native)
        assert panel.surface==words(0x4281b0,1)[0] and panel.cached_sum==words(0x424344,1)[0]
    # Same sum despite differing fields must preserve cached surface/text.
    ship.word[0x68//4]+=1;ship.word[0xa0//4]-=1
    panel.cached_sum=(ship.word[0x68//4]+ship.word[0xa0//4])&0xffffffff;actual.clear()
    assert lib.sniper_team_panel_draw(C.byref(panel))==0 and len(actual)==1 and actual[0][0]=='blit'
    print(f'900 team/bounty panel traces and cache states match original x86; {reuse} cached draws; same-sum invalidation quirk retained')

if __name__=="__main__":test_team()
