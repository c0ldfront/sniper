"""Complete original audio resource/dispatch path with deterministic Win32/COM seams."""
import pathlib,struct,json,sys
scenario=int(sys.argv[1]) if len(sys.argv)>1 else 0
import pefile
import original_resources
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EBP
root=pathlib.Path(__file__).resolve().parents[2];pe=pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x1000000);u.mem_write(0x400000,pe.get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
def signed(v):return v if v<2**31 else v-2**32
def hash(data):
 h=2166136261
 for b in data:h=((h^b)*16777619)&0xffffffff
 return h
heap=0x600000;allocations={}
def alloc(n):
 global heap
 p=heap;heap+=(n+31)&~15;allocations[p]=n;return p
resources={}
for key,raw in original_resources.payloads().items():
 if not key.startswith('WAVE-'):continue
 p=alloc(len(raw));u.mem_write(p,raw);resources[int(key.split('-')[1])]=(p,len(raw))
imports={}
for desc in pe.DIRECTORY_ENTRY_IMPORT:
 for imp in desc.imports:
  name=imp.name.decode() if imp.name else str(imp.ordinal);address=0x550000+len(imports)*16;imports[address]=name;put(imp.address,address)
methods={};device=0x560000;dv=0x561000;bv=0x562000;put(device,dv)
for table,indices in [(dv,[2,3,5,6]),(bv,[2,8,9,11,12,13,15,16,17,18,19,20])]:
 for m in indices:
  a=0x570000+len(methods)*16;methods[a]=(table,m);put(table+m*4,a)
buffers={};trace=[];uploads=[]
def emit(*args):trace.append(' '.join(map(str,args)))
def buffer_new(freq,n,source=None):
 ident=len(buffers)+1;ptr=0x580000+ident*16;put(ptr,bv)
 buffers[ptr]={'id':ident,'freq':freq,'bytes':n,'data':source if source else alloc(n),'status':0};return ptr
methods_n={2:1,8:2,9:2,11:8,12:4,13:2,15:2,16:2,17:2,18:1,19:5,20:1}
def hook(uc,a,size,user):
 sp=uc.reg_read(UC_X86_REG_ESP);ret=get(sp);args=lambda n:[get(sp+4+i*4) for i in range(n)];clean=0;result=0
 if a==0x4165f0:result=13
 elif a in imports:
  name=imports[a]
  if name=='DirectSoundCreate':
   g,out,unk=args(3);clean=12
   if scenario==5:emit('device_create_fail');result=0x80004005
   else:put(out,device);emit('device_create')
  elif name=='FindResourceA':instance,r,kind=args(3);result=r;clean=12
  elif name=='LoadResource':instance,r=args(2);result=r;clean=8
  elif name=='LockResource':r,=args(1);result=resources[r][0];clean=4
  else:raise AssertionError((hex(a),name))
 elif a in [0x416190,0x416b20,0x416210,0x416170,0x416620]:
  if a in [0x416190,0x416b20]:result=alloc(get(sp+4))
  elif a==0x416210:
   old,n=args(2);result=alloc(n)
   if old:u.mem_write(result,bytes(u.mem_read(old,min(n,allocations[old]))))
 elif a in methods:
  table,m=methods[a]
  if table==dv:
   n={2:1,3:4,5:3,6:3}[m];v=args(n);clean=n*4
   if m==6:emit('coop',v[2])
   elif m==2:emit('release_device')
   elif m==3:
    d=v[1];flags=get(d+4);nb=get(d+8);fmt=get(d+16);freq=get(fmt+4);ptr=buffer_new(freq,nb);put(v[2],ptr);emit('create',buffers[ptr]['id'],flags,nb,bytes(u.mem_read(fmt,18)).hex())
   else:
    old=buffers[v[1]]
    if scenario==1:
     emit('duplicate_fail',old['id']);uc.reg_write(UC_X86_REG_EAX,0x80004005);uc.reg_write(UC_X86_REG_ESP,sp+4+clean);uc.reg_write(UC_X86_REG_EIP,ret);return
    ptr=buffer_new(old['freq'],old['bytes'],old['data']);put(v[2],ptr);emit('duplicate',old['id'],buffers[ptr]['id'])
  else:
   v=args(methods_n[m]);clean=len(v)*4;b=buffers[v[0]];ident=b['id']
   if m==2:emit('release',ident)
   elif m==8:put(v[1],b['freq']);emit('get_frequency',ident,b['freq'])
   elif m==9:
    put(v[1],b['status']);emit('status',ident,b['status'])
    if scenario==3 and ident%3==0:result=0x80004005
   elif m==11:
    half=v[2]//2;put(v[3],b['data']);put(v[4],half);put(v[5],b['data']+half);put(v[6],v[2]-half);emit('lock',ident,v[1],v[2],v[7])
   elif m==19:
    data=bytes(u.mem_read(b['data'],b['bytes']));uploads.append(data);emit('unlock',ident,v[2],v[4],hash(data))
   elif m==12:
    if scenario!=4:b['status']=1|(4 if v[3] else 0)
    emit('play',ident,*v[1:])
   elif m==18:b['status']=0;emit('stop',ident)
   elif m==20:
    if scenario!=4:b['status']=0
    emit('restore',ident)
   else:emit({13:'position',15:'volume',16:'pan',17:'frequency'}[m],ident,signed(v[1]) if m in [15,16] else v[1])
 else:return
 if a in methods and methods[a][0]==bv:
  if scenario==2 and m==11 and ident==1:result=0x80004005
  if scenario==4 and m in [12,13,15,16,17,20]:result=0x80004005
 uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4+clean);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,hook)
def call(address,*args,this=0):
 put(0x900000,0x540000)
 for i,v in enumerate(args):put(0x900004+i*4,v)
 u.reg_write(UC_X86_REG_ESP,0x900000);u.reg_write(UC_X86_REG_EBP,0x910000);u.reg_write(UC_X86_REG_ECX,this);u.emu_start(address,0x540000,count=10000000)
 return u.reg_read(UC_X86_REG_EAX)
def sound(global_addr,request):call(0x40aae0,*request,this=get(global_addr))
def stop(global_addr):call(0x40aab0,this=get(global_addr))
call(0x40a5b0,87,0x20000)
if scenario==5:
 path=root/f'research/sniper87/runtime-audio/original-adapter-trace-{scenario}.txt';path.write_text('\n'.join(trace)+'\n');print('Original device failure trace retained');sys.exit(0)
call(0x403cf0)
emit('MARK','events');r=[0,0,1,0];sound(0x421900,r);stop(0x421904);sound(0x421900,r);sound(0x421904,r)
for a,v in [(0x4218e0,0x590000),(0x590048,1000000),(0x59004c,2000000),(0x42818c,640),(0x4282a5,100)]:put(a,v)
call(0x407a80,get(0x421908),1100,2200);call(0x407a80,get(0x4218ec),1100,2200);r=[-800,3125,0,-87]
obj=get(0x421908);next_voice=2 if get(obj+0x44)>2 else 0;put(obj+0x48,next_voice);buffers[get(obj+next_voice*4)]['status']=2;sound(0x421908,r)
emit('MARK','message');message=alloc(0x50);call(0x40a930,resources[171][0],this=message);call(0x40aae0,0,0,0,0,this=message)
emit('MARK','inactive');call(0x40a840,0);sound(0x421908,[0,0,0,0]);call(0x40aae0,0,0,0,0,this=message);call(0x40a840,1);call(0x40aae0,0,0,0,0,this=message)
emit('MARK','shutdown');call(0x40a8c0);call(0x40aa80,this=message);call(0x40a650)
path=root/f'research/sniper87/runtime-audio/original-adapter-trace-{scenario}.txt';path.write_text('\n'.join(trace)+'\n');print(f'Original complete audio trace: {len(trace)} events -> {path.name}')

for i,data in enumerate(uploads):(root/f"research/sniper87/runtime-audio/original-buffer-{scenario}-{i}.bin").write_bytes(data)
