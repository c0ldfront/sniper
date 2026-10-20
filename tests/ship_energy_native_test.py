"""Differential check of the local ship energy block, not full ship update."""
import ctypes as C
import pathlib
import random
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESI, UC_X86_REG_EDI
ROOT = pathlib.Path(__file__).resolve().parents[2]
p = pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'))
u = Uc(UC_ARCH_X86, UC_MODE_32)
u.mem_map(0x400000, 0x200000)
u.mem_write(0x400000, p.get_memory_mapped_image())
SHIP = 0x500000
cost_addresses = [0x428225, 0x428221, 0x428299, 0x428229]
class Energy(C.Structure):
    _fields_ = [('energy', C.c_int32), ('recharge', C.c_int32),
                ('maximum', C.c_int32), ('ability', C.c_int32*4)]
lib = C.CDLL(str(ROOT/'code/build/libship_energy.so'))
lib.sniper_ship_energy_step.argtypes = [C.POINTER(Energy), C.c_int32, C.POINTER(C.c_int32)]
lib.sniper_ship_energy_step.restype = None
def write(addr, val):
    u.mem_write(addr, struct.pack('<i', val))
def stop(uc, addr, size, data):
    if addr in (0x414431, 0x41443c): uc.emu_stop()
u.hook_add(UC_HOOK_CODE, stop)
rng = random.Random(87)
cases = []
# Exact cost boundary and ordering: later abilities see earlier deductions.
for energy in [0, 9, 10, 11, 19, 20, 39, 40]:
    for mask in range(16):
        cases.append((energy, 0, 100, [int(bool(mask & (1<<i))) for i in range(4)], 1, [10]*4))
for _ in range(2000):
    cases.append((rng.randrange(1000001), rng.randrange(10001), 1000000,
                  [rng.choice([0,1,2,-1]) for _ in range(4)], rng.randrange(51),
                  [rng.randrange(10001) for _ in range(4)]))
# Overflow coverage establishes instruction semantics beyond normal gameplay.
for _ in range(1000):
    vals = [rng.randrange(-2**31, 2**31) for _ in range(12)]
    cases.append((vals[0], vals[1], vals[2], vals[3:7], vals[7], vals[8:12]))
for case in cases:
    energy, recharge, maximum, flags, delta, costs = case
    u.mem_write(SHIP, bytes(0x200))
    for off, val in [(0x5c,energy),(0xc4,recharge),(0xc8,maximum)]: write(SHIP+off,val)
    for i, val in enumerate(flags): write(SHIP+0x88+4*i,val)
    for addr,val in zip(cost_addresses,costs): write(addr,val)
    u.reg_write(UC_X86_REG_ESI, SHIP)
    u.reg_write(UC_X86_REG_EDI, delta & 0xffffffff)
    u.emu_start(0x414376,0x414442,count=200)
    expected = [struct.unpack('<i',u.mem_read(SHIP+off,4))[0] for off in [0x5c,0x88,0x8c,0x90,0x94]]
    state = Energy(energy,recharge,maximum,(C.c_int32*4)(*flags))
    lib.sniper_ship_energy_step(C.byref(state),delta,(C.c_int32*4)(*costs))
    assert [state.energy,*state.ability] == expected, (case,expected,state.energy,list(state.ability))
print(f'{len(cases)} ship energy transitions match original x86')
