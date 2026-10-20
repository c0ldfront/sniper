#!/usr/bin/env python3
"""Full score callbacks/comparators: real x86 arithmetic and CRT string compare;
Win32 messages and printf are intercepted. Portable C is the production core."""
from pathlib import Path
import ctypes as C,random,struct,subprocess,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=Path(__file__).resolve().parents[2]
libpath=root/'code/build/libscore_dialog.so'
subprocess.run(['cc','-shared','-fPIC','-O2',str(root/'code/platform/score_dialog.c'),'-o',str(libpath)],check=True)
lib=C.CDLL(str(libpath))
class Player(C.Structure):_fields_=[('name',C.c_char*32),('squad',C.c_char*32),('wins',C.c_int16),('losses',C.c_int16),('points',C.c_int32)]
class Squad(C.Structure):_fields_=[('name',C.c_char*32),('wins',C.c_int16),('losses',C.c_int16),('points',C.c_int32),('members',C.c_int32)]
Cells=(C.c_char*64)*10
p=pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x700000,0x40000)
pack=lambda *v:struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v])
rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
signed=lambda n:C.c_int32(n).value
def string(a):return bytes(u.mem_read(a,256)).split(b'\0')[0].decode('latin1')
counts={'GetDlgItem':2,'SendMessageA':4,'GetPrivateProfileIntA':4,'MoveWindow':6,'GetSystemMetrics':1,'GetWindowRect':2,'WritePrivateProfileStringA':4,'EndDialog':2,'PostMessageA':4}
imports={}
for entry in p.DIRECTORY_ENTRY_IMPORT:
 for imp in entry.imports:
  name=imp.name.decode() if imp.name else ''
  if name in counts:
   addr=0x730000+len(imports)*16;imports[addr]=name;u.mem_write(imp.address,pack(addr))
events=[];rows=[];columns=[];widths=[]
def hook(u,a,n,d):
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x73ff00:u.emu_stop();return
 if a==0x416bc0:
  dest,fmt=rd(sp+4),string(rd(sp+8));v=struct.unpack('<d',u.mem_read(sp+12,8))[0] if fmt=='%.1f' else signed(rd(sp+12))
  out=(fmt%v).encode();u.mem_write(dest,out+b'\0');u.reg_write(UC_X86_REG_EAX,len(out));u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4);return
 if a not in imports:return
 name=imports[a];args=[rd(sp+4+i*4) for i in range(counts[name])];result=1
 if name=='GetDlgItem':result=args[1]
 elif name=='GetSystemMetrics':result=17
 elif name=='GetPrivateProfileIntA':result=args[2]
 elif name=='SendMessageA':
  h,msg,wp,lp=args
  if msg==0x101b:columns.append((wp,rd(lp),rd(lp+4),rd(lp+8),string(rd(lp+12)),rd(lp+20)))
  elif msg==0x1007:result=len(rows);rows.append([string(rd(lp+20))])
  elif msg==0x102e:
   col=rd(lp+8)
   while len(rows[wp])<=col:rows[wp].append('')
   rows[wp][col]=string(rd(lp+20))
  elif msg==0x101e:widths.append(lp)
  else:events.append((name,*args))
 elif name=='GetWindowRect':u.mem_write(args[1],pack(12,34,456,234))
 elif name=='WritePrivateProfileStringA':events.append((name,*[string(x) for x in args]))
 else:events.append((name,*args))
 u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4+4*len(args))
u.hook_add(UC_HOOK_CODE,hook)
u.mem_write(0x423f98,pack(0));u.mem_write(0x4230fc,pack(0))
def call(addr,*args):
 sp=0x72e000;u.mem_write(sp,pack(0x73ff00,*args));u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(addr,0x73ff00,count=100000);return signed(u.reg_read(UC_X86_REG_EAX))
rng=random.Random(87)
checks=0
for squad in (0,1):
 cls=Squad if squad else Player;func=getattr(lib,'sniper87_'+('squad' if squad else 'player')+'_score_compare');cellsfunc=getattr(lib,'sniper87_'+('squad' if squad else 'player')+'_score_cells')
 cols=10 if squad else 7;cmpaddr=0x40f410 if squad else 0x40e940;dlg=0x40ead0 if squad else 0x40e1f0
 def record():
  v=cls();v.name=rng.choice([b'Alpha',b'aLPHa',b'zulu',b'',b'Beta',b'\xffhigh']);v.wins=rng.choice([0,1,32767,-32768,-99,-101,rng.randrange(1,30000)]);v.losses=rng.randrange(-32768,32768);v.points=rng.randrange(-2**31,2**31)
  if squad:v.members=rng.choice([1,2,17,-7,200])
  else:v.squad=rng.choice([b'',b'Alpha',b'Beta',b'alpha'])
  return v
 for i in range(1200):
  a,b=record(),record();u.mem_write(0x700000,bytes(a));u.mem_write(0x700100,bytes(b));col=i%(cols+1)
  got=call(cmpaddr,0x700000,0x700100,col);expected=func(C.byref(a),C.byref(b),col)
  assert got==expected,(squad,col,got,expected,bytes(a),bytes(b));checks+=1
 for i in range(80):
  data=[record() for _ in range(3)]
  u.mem_write(0x700000,b''.join(bytes(x) for x in data));u.mem_write(0x42228c if squad else 0x422288,pack(0x700000));u.mem_write(0x422294 if squad else 0x422290,pack(3))
  rows.clear();columns.clear();events.clear();assert call(dlg,999,0x110,0,0)==1
  expected=[]
  for item in data:
   cells=Cells();assert cellsfunc(C.byref(item),cells);expected.append([bytes(c).split(b'\0')[0].decode('latin1') for c in cells][:cols])
  assert rows==expected,(squad,rows,expected)
  names=['Squad Name','#','Win','Lose','Rating','Points','Win/#','Lose/#','Pnts/#','Pnt/Win'] if squad else ['Player Name','Squad','Win','Lose','Rating','Points','Pnt/Win']
  assert [c[4] for c in columns]==names
  for k,c in enumerate(columns):assert c==(k,15,0,(70 if k==0 else 40 if k<6 else 50) if squad else (100 if k<2 else 50 if k<6 else 60),names[k],0 if squad or k<2 else 1 if k==2 else 2)
  widths.clear();w=rng.randrange(0,65536);assert call(dlg,999,5,0,(480<<16)|w)==1
  out=(C.c_uint16*10)();lib.sniper87_score_widths(squad,w,17,out);assert widths==list(out)[:cols];checks+=1
 events.clear();assert call(dlg,999,16,7,0)==1
 section='SquadScores' if squad else 'Scores'
 assert events==[('WritePrivateProfileStringA',section,k,v,'sniper.ini') for k,v in [('X','12'),('Y','34'),('Width','444'),('Height','200')]]+[('EndDialog',999,7)]
 for command in (1,2,99):
  events.clear();call(dlg,999,0x111,command,0);assert events==([('PostMessageA',999,16,0,0)] if command<3 else [])
 for col in range(cols):
  events.clear();u.mem_write(0x701000,pack(1010,1010,-108,0,col));call(dlg,999,0x4e,1010,0x701000)
  assert events==[('SendMessageA',1010,0x1030,col,cmpaddr)]
 assert call(dlg,999,0x999,0,0)==0
print(f'PASS {checks} native score comparisons: both full comparators, 480 populated records, columns, sizing, geometry and close paths')
