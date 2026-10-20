"""Process-lifetime previous/elapsed versus per-session accumulator native oracle."""
import ctypes as C
import pathlib,random,struct,subprocess,tempfile
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
class Clock(C.Structure):_fields_=[(n,C.c_uint32) for n in ['previous','elapsed','tick']]
with tempfile.TemporaryDirectory() as d:
    so=pathlib.Path(d)/'clock.so'
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(root/'code/src/frame_clock.c'),'-o',str(so)],check=True)
    lib=C.CDLL(str(so));lib.sniper_frame_clock_step.argtypes=[C.POINTER(Clock),C.c_uint32]
    lib.sniper_frame_clock_rebase.argtypes=[C.POINTER(Clock),C.c_uint32]
    lib.sniper_frame_clock_begin_session.argtypes=[C.POINTER(Clock),C.POINTER(Clock)]
    r=random.Random(0x402844);count=0
    for case in range(200):
        c=Clock(*[r.getrandbits(32) for _ in range(3)])
        for a,v in zip([0x42e198,0x4218c8,0x4218cc],[c.previous,c.elapsed,c.tick]):put(a,v)
        for cycle in range(3):
            # Settings40d41d..428 and activation40b0bb..c4 have same unsigned /10.
            if cycle!=1:
                ms=r.getrandbits(32);u.reg_write(UC_X86_REG_EAX,ms)
                u.emu_start(0x40b0bb,0x40b0c9,count=20)
                lib.sniper_frame_clock_rebase(C.byref(c),ms)
            before=Clock(c.previous,c.elapsed,c.tick)
            u.reg_write(UC_X86_REG_EDX,0);u.emu_start(0x402844,0x40284a,count=2)
            lib.sniper_frame_clock_begin_session(C.byref(c),C.byref(before))
            assert (c.previous,c.elapsed,c.tick)==(get(0x42e198),get(0x4218c8),get(0x4218cc))
            for frame in range(5):
                ms=r.getrandbits(32) if frame==0 else ((c.previous+r.choice([0,1,49,50,51,1000]))*10)&0xffffffff
                u.reg_write(UC_X86_REG_EAX,ms);u.emu_start(0x4020b5,0x4020fb,count=50)
                lib.sniper_frame_clock_step(C.byref(c),ms)
                assert (c.previous,c.elapsed,c.tick)==(get(0x42e198),get(0x4218c8),get(0x4218cc)),(case,cycle,frame)
                count+=1
print(f'PASS600 native session resets/400 activation rebases/{count} frame steps: previous+elapsed persist,tick resets,first delta uses prior process time')
