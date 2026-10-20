"""Actual frame/update/explosion/constructor/damage/death/RNG/notices chain."""
import ctypes as C
import pathlib,struct,random,sys
WEAPON="--weapon" in sys.argv
ACTION="--action" in sys.argv
GENERATE="--generate" in sys.argv
FRAME="--frame" in sys.argv or GENERATE
CONTROL="--control" in sys.argv or FRAME
INPUT="--input" in sys.argv or CONTROL
PICKUPS="--pickups" in sys.argv
SIMULATION="--simulation" in sys.argv or PICKUPS
key_bits=0
mine=0
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x900000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
def string(a):return bytes(u.mem_read(a,128)).split(b'\0')[0].decode()
class Ship(C.Structure):_fields_=[('word',C.c_int32*134)]
class Projectile(C.Structure):_fields_=[('word',C.c_int32*18)]
class Hud(C.Structure):_fields_=[('ticks',C.c_int32*4),('text',(C.c_char*128)*4),('color',C.c_int32*4)]
lib=C.CDLL(str(ROOT/'code/build/liboffline_combat_test.so'))
lib.test_offline_combat.argtypes=[C.c_void_p]*8
lib.test_offline_combat_weapon.argtypes=[C.c_void_p]*9
lib.test_offline_simulation_pickups.argtypes=[C.c_void_p]*9
lib.test_offline_control.argtypes=[C.c_void_p]*10
control_result=(C.c_int32*64)();flight_sounds=[];native_fields=[];posted=0;generated_count=0
pickups=(C.c_int32*10)()
config=(C.c_int32*72)();lib.sniper_local_config(config)
ships=(Ship*2)();objects=(Projectile*32)();grid=(C.c_uint8*(1024*1024))();coverage=(C.c_uint8*8192)();hud=Hud();indices=(C.c_int32*32)();log=(C.c_int32*2048)();ne=[];allocated=2;seen=set()
def hook(uc,a,size,data):
    global allocated,posted,generated_count
    if FRAME:
        if a==0x4050ce:
            sp=uc.reg_read(UC_X86_REG_ESP)
            for off in (0x20,0x1c,0x10,0x18):put(sp+off,0)
            put(sp+0x14,args[5]);uc.reg_write(UC_X86_REG_EIP,0x405f75);return
        if a==0x540400:
            posted+=1;sp=uc.reg_read(UC_X86_REG_ESP)
            assert get(sp+8)==0x401
            uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+20);return
    if CONTROL:
        if not FRAME and a in (0x405f97,0x406a9f):uc.emu_stop();return
        if a==0x406922:
            sp=uc.reg_read(UC_X86_REG_ESP)
            native_fields[:]=[C.c_int32(uc.reg_read(UC_X86_REG_ESI)).value]+[get(sp+i) for i in (0x20,0x1c,0x14,0x10,0x18)]
        if a in (0x540200,0x540300,0x40aae0,0x40aab0):
            sp=uc.reg_read(UC_X86_REG_ESP);clean=0
            if a==0x540200:
                ptr=get(sp+8)
                for off,v in [(8,args[12]),(12,args[13]),(32,args[3]),(40,args[4])]:put(ptr+off,v)
                clean=8;uc.reg_write(UC_X86_REG_EAX,0)
            elif a==0x540300:clean=52
            else:
                slot=(uc.reg_read(UC_X86_REG_ECX)-0x560000)//256
                flight_sounds.append((slot,int(a==0x40aae0)))
                if a==0x40aae0:clean=16
            uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean);return
    seen.add(a) if a in [0x413510,0x412b60,0x414b10,0x411540,0x404680,0x407b50,0x411620] else None
    if a==0x540100:
        sp=uc.reg_read(UC_X86_REG_ESP);uc.reg_write(UC_X86_REG_EAX,1000);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4);return
    if a==0x540000:
        sp=uc.reg_read(UC_X86_REG_ESP);vk=get(sp+4);pressed=bool(key_bits&({17:1,16:2,9:4,38:8,40:16,37:32,39:64}[vk])) if INPUT else mine;uc.reg_write(UC_X86_REG_EAX,0x8000 if pressed else 0);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+8);return
    if a not in [0x416b20,0x416620,0x407a80,0x416bc0]:return
    sp=uc.reg_read(UC_X86_REG_ESP);value=0
    if a==0x416b20:
        if FRAME and get(sp+4)==20:
            ne.append((5,generated_count,0,0));value=0x550000+generated_count*20;generated_count+=1
        else:
            assert get(sp+4)==72
            ne.append((1,allocated,0,0));value=0x500000+allocated*72;allocated+=1;assert allocated<=32
    elif a==0x416620:
        address=get(sp+4)
        ne.append((4,(address-0x550000)//20,0,0) if 0x550000<=address<0x550028 else (2,(address-0x500000)//72,0,0))
    elif a==0x407a80:ne.append((3,get(sp+4),get(sp+8),get(sp+12)))
    else:
        fmt=string(get(sp+8));fmt_args=[string(get(sp+12)),get(sp+16)]
        if fmt.count('%s')==2:fmt_args.append(string(get(sp+20)))
        text=(fmt%tuple(fmt_args)).encode();uc.mem_write(get(sp+4),text+b'\0');value=len(text)
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(UC_HOOK_CODE,hook)
for a,v in [(0x429f48,0x600000),(0x429f50,0x700000),(0x42de7c,32),(0x429318,0),(0x429328,2),(0x4218e0,0x510000),(0x424348,0x520000),(0x520000,0x520100),(0x520114,0x411540),(0x52012c,0x411620),(0x4218f8,104),(0x4218ec,101),(0x4218f4,102),(0x4218f0,103)]:put(a,v)
# Impact sound handle used by update is established below from its global.
for i in range(2):put(0x429338+i*4,0x510000+i*536);put(0x4283e8+i*4,0x510000+i*536)
rng=random.Random(7711)
for case in range(360):
    C.memset(ships,0,C.sizeof(ships));C.memset(objects,0x55,C.sizeof(objects));C.memset(grid,0,len(grid));C.memset(coverage,255,8192);C.memset(C.byref(hud),0x41,C.sizeof(hud))
    for i,s in enumerate(ships):
        C.memmove(C.byref(s),f'Pilot{i}'.encode()+b'\0',7)
        for off,val in [(0x44,i),(0x48,400000+i*80000),(0x4c,400000),(0x5c,[1,200000,1000000][case%3]),(0x68,i),(0x6c,0 if case%4==0 else -1),(0xa0,5),(0x80,100),(0xc4,10000),(0x108,1 if case%11==0 else 0)]:s.word[off//4]=val
    for i in range(2):
        p=objects[i];C.memset(C.byref(p),0,72)
        for off,val in [(0,1),(4,400000+i*80000),(8,400000),(0xc,1000),(0x14,0),(0x1c,i+1),(0x20,1),(0x30,0),(0x34,3 if i==0 and case%2==0 else 1),(0x38,case%3),(0x3c,1),(0x40,0),(0x44,case%4)]:p.word[off//4]=val
    if SIMULATION:
        for s in ships:
            s.word[0x50//4]=0;s.word[0x54//4]=0;s.word[0x74//4]=100;s.word[0xc8//4]=2000000
            s.word[0x60//4]=30 if case%7==0 else 0
    if ACTION or INPUT:
        for off in [0xd0,0xd4,0xdc,0xd8,0xec,0xf0,0xf4,0xf8,0xfc]:ships[0].word[off//4]=2
        ships[0].word[0x5c//4]=2000000
    if CONTROL:
        for off,val in [(0x40,case%4),(0xcc,66),(0xe0,40),(0xe4,3000),(0x6c,-1),(0x104,int(case%4==0)),(0x214,case%12)]:ships[0].word[off//4]=val
        if case%13==0:ships[0].word[0x5c//4]=-1
    if case%2==0 and not FRAME:grid[25*1024+25]=1
    for i in range(4):hud.ticks[i]=rng.choice([0,10]);hud.color[i]=3
    values=(C.c_int32*10)(rng.choice([1,2,5,10]),rng.randrange(0x7fffffff),2,123,0,0,0)
    if PICKUPS:
        for i in range(2):
            for j,v in enumerate([7,40+i,40,[1,20][i],1]):pickups[i*5+j]=v
            grid[40*1024+40+i]=255;put(0x429288+4*i,0x550000+20*i)
        u.mem_write(0x550000,bytes(pickups))
    u.mem_write(0x4281c9,bytes(config));u.mem_write(0x500000,bytes(objects));u.mem_write(0x510000,bytes(ships));u.mem_write(0x600000,bytes(grid));u.mem_write(0x700000,bytes(coverage))
    u.mem_write(0x424330,bytes(hud.ticks));u.mem_write(0x4287e8,bytes(hud.text));u.mem_write(0x428a50,bytes(hud.color));u.mem_write(0x429748,bytes(2048))
    for i in range(2):put(0x429748+4*i,0x500000+72*i)
    for a,v in [(0x4289f0,2),(0x4218c8,values[0]),(0x422f78,values[1]),(0x428178,123),(0x429738,0)]:put(a,v)
    allocated=2;ne.clear();u.reg_write(UC_X86_REG_ESP,0x800000)
    if SIMULATION:
        put(0x4353f4,0x540100);put(0x428a08,2 if PICKUPS else 0);put(0x42930c,0)
        u.emu_start(0x4021bf,0x402338,count=2000000)
        args=(C.c_int32*14)(-97)
        if PICKUPS:
            lib.test_offline_simulation_pickups(ships,objects,grid,coverage,C.byref(hud),values,indices,log,pickups)
            assert bytes(pickups)==bytes(u.mem_read(0x550000,40)),(case,'pickups')
            assert values[7]==get(0x428a08),(case,'pickup count')
            assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),(case,'map')
        else:lib.test_offline_combat_weapon(ships,objects,grid,coverage,C.byref(hud),values,indices,log,args)
        assert bytes(coverage)==bytes(u.mem_read(0x700000,8192)),(case,'coverage')
    elif CONTROL:
        key_bits=case%128
        args=(C.c_int32*14)(-94 if GENERATE else -95 if FRAME else -96,key_bits,case%2,case%16,[-1,0,9000,18000,27000][case%5],case%3,case%70,320+(case%9)*8,640,123 if case%17==0 else 0,case%11,case%110,rng.randrange(65536),rng.randrange(65536))
        put(0x4354b8,0x540000);put(0x435580,0x540200);put(0x52010c,0x540300)
        for address,value in [(0x429300,0),(0x4283d8,0),(0x4292fc,0),(0x422278,args[2]),(0x800014,args[5]),(0x42229c,args[6]),(0x4222a0,args[7]),(0x42434c,args[8]),(0x422248,args[9]),(0x42224c,7),(0x4222a4,17),(0x4222a8,-5),(0x42930c,1),(0x42833c,args[10]),(0x4218cc,args[11]),(0x42e144,0)]:put(address,value)
        for address in (0x4281c0,0x428a68,0x428a00,0x428a70):put(address,0)
        for slot in range(3):put(0x4218fc+4*slot,0x560000+slot*256)
        for off in (0x20,0x1c,0x10,0x18):put(0x800000+off,0)
        u.mem_write(0x422244,b'A');u.mem_write(0x4292c8,struct.pack('<13I',*([0x12345678]*13)))
        u.reg_write(UC_X86_REG_FPCW,0x37f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
        alive=ships[0].word[0x5c//4]>=0;flight_sounds.clear();native_fields.clear()
        if FRAME:
            config[(0x4282a9-0x4281c9)//4]=4;u.mem_write(0x4281c9,bytes(config))
            for address,value in [(0x4353f4,0x540100),(0x4354d0,0x540400),(0x520108,0x411450),(0x520128,0x411600),(0x520008,100-501 if GENERATE else 100),(0x428a08,0),(0x42930c,100),(0x42e198,100-values[0]),(0x4218cc,0),(0x4218d0,0),(0x428160,0),(0x42def0,args[11]),(0x429f58,0),(0x4289f4,0),(0x42818c,640),(0x42434c,480)]:put(address,value)
            # Input geometry uses original vertical extent too.
            args[8]=480;put(0x4222a0,args[7]);u.reg_write(UC_X86_REG_EAX,1000);posted=0;generated_count=0
            u.mem_write(0x550000,bytes(320))
        u.emu_start(0x4020b5 if FRAME else 0x405f75,0x402468 if FRAME else 0x406aa6,count=2000000)
        lib.test_offline_control(ships,objects,grid,coverage,C.byref(hud),values,indices,log,args,control_result)
        assert list(values)[7:10]==[get(a) for a in [0x429300,0x4283d8,0x4292fc]],(case,'control globals')
        assert list(control_result)[:6]==[get(a) for a in (0x42833c,0x422248,0x42224c,0x4222a4,0x4222a8)]+[u.mem_read(0x422244,1)[0]],(case,'cleanup')
        assert list(control_result)[7:12]==[get(a) for a in (0x4281c0,0x428a68,0x428a00,0x428a70,0x4292fc)],(case,'position')
        assert control_result[12]==(True if FRAME else alive)
        assert [(control_result[14+i],control_result[17+i]) for i in range(control_result[13])]==flight_sounds
        assert bytes(control_result)[80:132]==bytes(u.mem_read(0x4292c8,52))
        if alive:assert list(control_result)[33:39]==native_fields,(case,'scratch')
        if FRAME:
            expected=[get(a) for a in (0x429f58,0x4289f4,0x42e198,0x4218c8,0x4218cc,0x42de80,0x424324)]+[posted,get(0x42def0),get(0x42930c)]
            assert list(control_result)[40:50]==expected,(case,'frame',list(control_result)[40:50],expected)
            assert bytes(coverage)==bytes(u.mem_read(0x700000,8192)),(case,'coverage')
            assert bytes(grid)==bytes(u.mem_read(0x600000,len(grid))),(case,'frame map')
            assert bytes(control_result)[200:220]==bytes(u.mem_read(0x550000,20)),(case,'generated pickup')
            assert control_result[55]==get(0x428a08) and control_result[56]==get(0x520008),(case,'generation state')
    elif INPUT:
        key_bits=case%8
        args=(C.c_int32*14)(-98,key_bits,case%2,case%16,[-1,0,9000,18000,27000][case%5],case%3,case%70,320+(case%9)*8,640)
        put(0x4354b8,0x540000)
        for address,value in [(0x429300,0),(0x4283d8,0),(0x4292fc,0),(0x422278,args[2]),(0x4292e8,args[3]),(0x4292f0,args[4]),(0x800014,args[5]),(0x42229c,args[6]),(0x4222a0,args[7]),(0x42434c,args[8])]:put(address,value)
        u.emu_start(0x406332,0x406922,count=2000000)
        lib.test_offline_combat_weapon(ships,objects,grid,coverage,C.byref(hud),values,indices,log,args)
        assert list(values)[7:10]==[get(a) for a in [0x429300,0x4283d8,0x4292fc]],(case,'input globals')
    elif ACTION:
        mine=case%3==0;put(0x4354b8,0x540000);put(0x4292e8,0)
        for address in [0x429300,0x4283d8,0x4292fc]:put(address,0)
        u.reg_write(UC_X86_REG_EBX,0);u.reg_write(UC_X86_REG_EBP,1);u.reg_write(UC_X86_REG_EDI,case%8)
        u.emu_start(0x406431,0x406922,count=2000000)
        args=(C.c_int32*14)(-99,1,case%8,mine)
        lib.test_offline_combat_weapon(ships,objects,grid,coverage,C.byref(hud),values,indices,log,args)
        assert list(values)[7:10]==[get(a) for a in [0x429300,0x4283d8,0x4292fc]],(case,'action globals')
    elif WEAPON:
        args=(C.c_int32*14)(case%6,case%3,1,0,case%4,(case//6)%2,values[0],case*97%40000,400000,400000,100,-100,1,1)
        put(0x800000,0x810000);u.mem_write(0x800004,bytes(args))
        u.emu_start(0x409d70,0x810000,count=2000000)
        lib.test_offline_combat_weapon(ships,objects,grid,coverage,C.byref(hud),values,indices,log,args)
    else:
        u.emu_start(0x40229b,0x4022f2,count=2000000)
        lib.test_offline_combat(ships,objects,grid,coverage,C.byref(hud),values,indices,log)
    assert bytes(objects)==bytes(u.mem_read(0x500000,C.sizeof(objects))),(case,'objects')
    assert bytes(ships)==bytes(u.mem_read(0x510000,C.sizeof(ships))),(case,'ships')
    for data,a in [(hud.ticks,0x424330),(hud.text,0x4287e8),(hud.color,0x428a50)]:assert bytes(data)==bytes(u.mem_read(a,C.sizeof(data))),(case,'hud',hex(a))
    assert list(values)[1:6]==[get(0x422f78),get(0x4289f0),get(0x428178),get(0x429738),allocated],(case,'globals',list(values))
    for i in range(32):assert indices[i]==((get(0x429748+i*4)-0x500000)//72 if get(0x429748+i*4) else -1),(case,'list',i)
    ce=[tuple(log[i*4+j] for j in range(4)) for i in range(values[6])];assert ce==ne,(case,ce,ne)
required={0x413510,0x412b60,0x414b10,0x411540,0x404680,0x411620}
if not WEAPON:required.add(0x407b50)
if ACTION or INPUT:required={0x412b60}
if CONTROL:required|={0x414b10,0x411540,0x404680}
assert required<=seen,seen
print(('Simulation stage: ' if SIMULATION else '')+('Input dispatch: ' if INPUT else '')+('Action dispatch: ' if ACTION else '')+('Weapon creation: ' if WEAPON else '')+'360 integrated offline combat frames match original objects,ships,HUD,RNG,shake,groups,list and allocator/audio effects')

if CONTROL:print("Full ordinary offline control includes actual damage/death, projectile creation and exhaust; speed-exempt")

if FRAME:print("Combined clock,input,team,simulation,camera pass; generation included; earlier input commands bypassed")

if GENERATE:assert generated_count==1;print("Actual timed pickup generation and same-frame aging/map writes verified")
