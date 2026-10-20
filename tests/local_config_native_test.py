"""Verify every offline config DWORD against original constructor instructions."""
import ctypes as C
import pathlib
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EBP
ROOT=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000)
u.mem_write(0x400000,pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
u.reg_write(UC_X86_REG_EBP,0x500000)
u.mem_write(0x4281c9,bytes([0xa5])*288)
u.emu_start(0x4110ab,0x411335,count=300)
lib=C.CDLL(str(ROOT/'code/build/liblocal_config.so'))
class Config(C.Structure):_fields_=[('word',C.c_int32*72)]
lib.sniper_local_config.argtypes=[C.POINTER(Config)]
c=Config();lib.sniper_local_config(C.byref(c))
assert bytes(c)==bytes(u.mem_read(0x4281c9,288))
print('All 72 offline configuration DWORDs match original constructor instructions')
