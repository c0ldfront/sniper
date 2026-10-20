"""4013d0 liveness,401430 reset,401280 already-drained shutdown comparisons."""
import ctypes as C,pathlib,random,struct,subprocess,tempfile
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
root=pathlib.Path(__file__).resolve().parents[2]
bridge='''#include "network_lifecycle.h"
uint32_t lifecycle(int mode,uint32_t last,int connected,int initialized,uint32_t(*clock)(void*),int32_t(*send)(void*,const uint8_t*,int32_t),void(*close)(void*),uint32_t*out){SniperNetworkTransport t={0};t.host.clock_ms=clock;t.host.send=send;t.host.close=close;t.sender.connected=connected;t.last_receive_tick=last;if(mode){sniper_network_transport_shutdown(&t,0,0);*out=t.sender.connected;return 0;}int r=sniper_network_transport_alive(&t,initialized);*out=t.last_receive_tick;return r;}
'''
with tempfile.NamedTemporaryFile(suffix='.c',mode='w') as f:
    f.write(bridge);f.flush();subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'code/src'),f.name,*[str(root/'code/src'/f'network_{n}.c') for n in ['send','receive','transport','lifecycle']],'-o',str(root/'code/build/libnetwork_lifecycle_test.so')],check=True)
Clock=C.CFUNCTYPE(C.c_uint32,C.c_void_p);Send=C.CFUNCTYPE(C.c_int32,C.c_void_p,C.c_void_p,C.c_int32);Close=C.CFUNCTYPE(None,C.c_void_p)
lib=C.CDLL(str(root/'code/build/libnetwork_lifecycle_test.so'));lib.lifecycle.argtypes=[C.c_int,C.c_uint32,C.c_int,C.c_int,Clock,Send,Close,C.POINTER(C.c_uint32)]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000);u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
tick=0;step=17;calls=0;events=[]
@Clock
def clock(_):
    global calls
    v=(tick+calls*step)&0xffffffff;calls+=1;return v
@Send
def send(_,p,n):events.append(('send',C.string_at(p,n),calls));return n
@Close
def close(_):events.append(('close',calls))
def hook(uc,a,z,_):
    sp=uc.reg_read(UC_X86_REG_ESP);cleanup=0
    if a==0x700000:value=clock(None)
    elif a==0x41614c:
        n=get(sp+12);events.append(('send',bytes(uc.mem_read(get(sp+8),n)),calls));value=n;cleanup=16
    elif a==0x41611c:events.append(('close',calls));value=0;cleanup=4
    else:return
    uc.reg_write(UC_X86_REG_EAX,value);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+cleanup)
u.hook_add(UC_HOOK_CODE,hook);put(0x4353f4,0x700000);r=random.Random(401280)
for case in range(1800):
    mode=case>=1200;last=r.randrange(2**32);connected=r.randrange(2);initialized=r.randrange(2);tick=r.randrange(2**32);step=r.randrange(1,101);calls=0;events=[]
    u.mem_write(0x500000,b'\0'*0x4000);put(0x500014,last);put(0x5008b0,5 if connected else -1);put(0x421030,initialized);put(0x800000,0x800100)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_ECX,0x500000);u.emu_start(0x401280 if mode else 0x4013d0,0x800100,count=100000)
    expected=u.reg_read(UC_X86_REG_EAX);native_calls=calls;native_events=events[:];native_last=get(0x500014);calls=0;events=[];out=C.c_uint32()
    result=lib.lifecycle(mode,last,connected,initialized,clock,send,close,C.byref(out))
    assert calls==native_calls and events==native_events,(case,calls,native_calls,events,native_events)
    if not mode:assert result==expected and out.value==native_last
    else:assert out.value==0
print('PASS1200 original liveness and600 drained shutdown comparisons: signed rebase, unsigned timeout, clock reads, four close datagrams and delay')
