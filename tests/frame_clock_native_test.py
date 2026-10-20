"""Compare recovered clock with original x86, including timer/accumulator wrap."""
import ctypes as C
import pathlib
import random
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX

ROOT = pathlib.Path(__file__).resolve().parents[2]
pe = pefile.PE(str(ROOT / 'code/build/assets/files/SNIPER.EXE'))
u = Uc(UC_ARCH_X86, UC_MODE_32)
u.mem_map(0x400000, 0x200000)
u.mem_write(0x400000, pe.get_memory_mapped_image())

class Clock(C.Structure):
    _fields_ = [('previous', C.c_uint32), ('elapsed', C.c_uint32), ('tick', C.c_uint32)]

lib = C.CDLL(str(ROOT / 'code/build/libframe_clock.so'))
for name in ['sniper_frame_clock_step', 'sniper_frame_clock_rebase']:
    getattr(lib, name).argtypes = [C.POINTER(Clock), C.c_uint32]

addresses = [0x42e198, 0x4218c8, 0x4218cc]
def check(state, ms, start, end, function):
    for address, value in zip(addresses, state):
        u.mem_write(address, struct.pack('<I', value))
    u.reg_write(UC_X86_REG_EAX, ms)
    u.emu_start(start, end, count=100)
    expected = tuple(struct.unpack('<I', u.mem_read(a, 4))[0] for a in addresses)
    clock = Clock(*state)
    function(C.byref(clock), ms)
    actual = (clock.previous, clock.elapsed, clock.tick)
    assert actual == expected, (state, ms, actual, expected)
    return actual

rng = random.Random(87)
cases = [(p, e, t, ms)
         for p in [0, 1, 100, 429496729, 0x7fffffff, 0x80000000, 0xffffffff]
         for e in [0, 50]
         for t in [0, 0xfffffff0]
         for ms in [0, 9, 10, 499, 500, 509, 510, 999, 1000, 0xfffffff9, 0xffffffff]]
cases += [tuple(rng.getrandbits(32) for _ in range(4)) for _ in range(2000)]
for p, e, t, ms in cases:
    check((p, e, t), ms, 0x4020b5, 0x4020fb, lib.sniper_frame_clock_step)
    check((p, e, t), ms, 0x40b0bb, 0x40b0c9, lib.sniper_frame_clock_rebase)
state = (429496720, 0, 0xfffffff0)
for ms in [0xfffffff0, 0xffffffff, 0, 9, 10, 510, 5000, 5010]:
    state = check(state, ms, 0x4020b5, 0x4020fb, lib.sniper_frame_clock_step)
print(f'{len(cases)} step/rebase pairs and 8 sequential wrap/stall cases match original x86')
