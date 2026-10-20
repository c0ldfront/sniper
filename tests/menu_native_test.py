#!/usr/bin/env python3
"""Finite MENU103 inventory and native WM_INITMENU oracle; no network."""
from pathlib import Path
import struct,json,random,pefile
import original_resources
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=Path(__file__).resolve().parents[2];p=pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE'));mem=p.get_memory_mapped_image()
b=original_resources.payloads()['4-103-1033'];pos=4+struct.unpack_from('<H',b,2)[0];items=[]
def word():
 global pos
 v=struct.unpack_from('<H',b,pos)[0];pos+=2;return v
def label():
 out=[]
 while True:
  v=word()
  if not v:return ''.join(out)
  out.append(chr(v))
def menu(path=''):
 while True:
  flags=word();command=None if flags&16 else word();name=label();full=path+'/'+name
  if command:items.append(dict(command=command,label=full,resource_flags=flags))
  if flags&16:menu(full)
  if flags&128:return
menu();assert pos==len(b)
for item in items:
 cmd=item['command'];item['native_branch']=hex(0x40b15a if cmd==102 else 0x40bb22 if cmd==103 else 0x40bb59 if cmd==40002 else struct.unpack_from('<I',mem,0xce04+4*(cmd-40003))[0])
for item in items:
 cmd=item['command']
 if cmd in [40004,40020,40005,40006,40007,40018,40019]:
  item.update(implementation='platform/display.c:sniper_display_command',verification='menu_runtime_test.c resolution dispatch and native WM_INITMENU oracle')
 elif cmd in [40021,40023,40024,40025,40026,40027,40028,40029,40032]:
  item.update(implementation='src/options.c:sniper_options_command + platform/settings.c',verification='options_native_test.py; menu_runtime_test.c option dispatch and native WM_INITMENU oracle')
 elif cmd in [40012,40014,40030,40031]:
  item.update(implementation='platform/network_commands.c:sniper87_network_command',verification='network_commands_native_test.py; score_dialog/aux_dialog tests; menu enable reachability')
 else:
  item.update(implementation='platform/startup.c:window_proc',verification='menu enable reachability plus dedicated startup/dialog tests; Contents operational verification separate')
 item['gating']='transport alive' if cmd in [103,40012,40014,40030,40031] else 'transport absent/dead' if cmd==102 else 'Sound nonzero' if cmd in [40025,40027] else 'mode enumerated' if cmd in [40004,40020,40005,40006,40007,40018,40019] else 'always enabled by unchanged resource state'
out=root/'research/sniper87/menu-command-matrix.json';out.write_text(json.dumps({'resource':'4-103-1033.bin','commands':items,'hidden':[{'command':40009,'native_branch':'0x40bde9','route':'editor; also Shift+Ctrl+40008'},{'command':40013,'native_branch':'0x40c2f1','route':'download executable;network_commands.c'}],'default_holes':[40010,40011,40015,40022]},indent=2)+'\n')
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,mem);u.mem_map(0x700000,0x30000)
pk=lambda *v:struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]);rd=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
counts={'GetMenu':1,'CheckMenuItem':3,'EnableMenuItem':3};imports={}
for dll in p.DIRECTORY_ENTRY_IMPORT:
 for imp in dll.imports:
  name=imp.name.decode() if imp.name else ''
  if name in counts:a=0x720000+len(imports)*16;imports[a]=name;u.mem_write(imp.address,pk(a))
events=[];health=1;cleanup=[]
def hook(u,a,n,d):
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x40cddb:u.emu_stop();return
 if a in [0x4013d0,0x401280,0x416620,0x401080]:
  if a!=0x4013d0:cleanup.append(hex(a))
  u.reg_write(UC_X86_REG_EAX,health if a==0x4013d0 else 0);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4);return
 if a not in imports:return
 name=imports[a];v=[rd(sp+4+i*4) for i in range(counts[name])]
 if name!='GetMenu':events.append([name,v[1],v[2]])
 u.reg_write(UC_X86_REG_EAX,777);u.reg_write(UC_X86_REG_EIP,rd(sp));u.reg_write(UC_X86_REG_ESP,sp+4+4*len(v))
u.hook_add(UC_HOOK_CODE,hook)
opts=[0x422278,0x422280,0x42e018,0x42e000,0x42e53c,0x42e144,0x42def4,0x42e004,0x422284]
commands=[40021,40023,40024,40025,40026,40027,40028,40029,40032]
modes=[40004,40020,40005,40006,40007,40018,40019];widths=[320,320,640,800,1024,1280,1600];heights=[200,240,480,600,768,1024,1200]
rng=random.Random(103);cases=[]
for case in range(144):
 mode=case%9;w=widths[mode] if mode<7 else 640 if mode==7 else 123;h=heights[mode] if mode<7 else 123;mask=rng.randrange(128);values=[rng.choice([0,1,-1,2]) for _ in opts];connected=case%3;health=int(connected!=2)
 for addr,value in zip(opts,values):u.mem_write(addr,pk(value))
 u.mem_write(0x422250,pk(w,h));u.mem_write(0x42225c,pk(*[(mask>>i)&1 for i in range(7)]));u.mem_write(0x422238,pk(0x701000 if connected else 0))
 frame=0x71e000;u.mem_write(frame+8,pk(999));u.reg_write(UC_X86_REG_EBP,frame);u.reg_write(UC_X86_REG_ESP,frame-0x400);events.clear();cleanup.clear();u.emu_start(0x40c8b8,0x40cddb,count=3000)
 checks={i:v for n,i,v in events if n=='CheckMenuItem'};enable={i:v for n,i,v in events if n=='EnableMenuItem'}
 assert all(checks[c]==(8 if values[i] else 0) for i,c in enumerate(commands))
 assert all(checks[c]==(8 if w==widths[i] and (i>1 or h==heights[i]) else 0) for i,c in enumerate(modes))
 assert all(enable[c]==(0 if mask&(1<<i) else 1) for i,c in enumerate(modes))
 assert enable[40025]==enable[40027]==(0 if values[4] else 1)
 assert all(enable[c]==(0 if connected==1 else 1) for c in [103,40012,40014,40030,40031])
 assert enable[102]==(1 if connected==1 else 0)
 assert cleanup==(['0x401280','0x416620','0x401080'] if connected==2 else [])
 cases.append(dict(width=w,height=h,supported=mask,options=values,connection=connected,events=events[:],cleanup=cleanup[:]))
(root/'research/sniper87/menu-init-oracle.json').write_text(json.dumps(cases,indent=2)+'\n')
# Plain numeric oracle is consumed by actual startup runtime test.
lines=[]
for c in cases:
 checks={i:v for n,i,v in c['events'] if n=='CheckMenuItem'}
 enabled={i:v for n,i,v in c['events'] if n=='EnableMenuItem'}
 flags=[checks.get(item['command'],0)|enabled.get(item['command'],0) for item in items]
 lines.append(' '.join(map(str,[c['width'],c['height'],c['supported'],c['connection'],*c['options'],*flags])))
(root/'research/sniper87/menu-init-oracle.txt').write_text('\n'.join(lines)+'\n')
print(f'PASS144 native WM_INITMENU paths;{len(items)} real resource commands; disconnected/live/stale connection cleanup,all16 checks and15 enables')
route={'target':0,'hit':False}
def route_hook(u,a,n,d):
 if a==route['target']:route['hit']=True;u.emu_stop()
u.hook_add(UC_HOOK_CODE,route_hook)
routes={i['command']:int(i['native_branch'],16) for i in items}
routes.update({40009:0x40bde9,40013:0x40c2f1,40010:0x40cddb,40011:0x40cddb,40015:0x40cddb,40022:0x40cddb,65535:0x40cddb})
for command,target in routes.items():
 for high in [0,0x12340000]:
  route.update(target=target,hit=False);u.mem_write(frame+0x10,pk(command|high));u.reg_write(UC_X86_REG_EBP,frame);u.reg_write(UC_X86_REG_ESP,frame-0x400);u.emu_start(0x40b11d,target,count=100)
  # Unicorn until address stops before the code-hook; PC still proves dispatch.
  assert u.reg_read(UC_X86_REG_EIP)==target,(command,target)
print('PASS68 native low-word command dispatches including27 menu items,hidden editor/download and default holes')
