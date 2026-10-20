"""Compare reconstructed map placement with the original x86 routine in Unicorn."""
import ctypes
import pathlib
import random
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP

ROOT = pathlib.Path(__file__).resolve().parents[2]
PE = pefile.PE(str(ROOT / 'code/build/assets/files/SNIPER.EXE'))
uc = Uc(UC_ARCH_X86, UC_MODE_32)
uc.mem_map(0x400000, 0x200000)
uc.mem_write(0x400000, PE.get_memory_mapped_image())
MAP, STACK, DESC, STOP = 0x1000000, 0x1200000, 0x1300000, 0x1400000
for addr, size in [(MAP,1048576),(STACK,65536),(DESC,65536),(STOP,4096)]:
    uc.mem_map(addr,size)
uc.mem_write(0x429f48,struct.pack('<I',MAP))
class Size(ctypes.Structure):
    _fields_ = [('width',ctypes.c_int32),('height',ctypes.c_int32)]
sizes = (Size*256)()
for tile,side in [(216,16),(217,32),(218,64),(219,96)]:
    sizes[tile]=Size(side,side)
    p=DESC+tile*256
    uc.mem_write(0x42da18+tile*4,struct.pack('<I',p))
    uc.mem_write(p+0x84,struct.pack('<ii',side,side))
lib=ctypes.CDLL(str(ROOT/'code/build/libmap.so'))
lib.sniper_map_place.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(Size),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint8]
lib.sniper_map_place.restype=ctypes.c_int
rng=random.Random(87)
cases=0
for old in [0,1,215,216,217,218,219]:
    for new in [0,1,215,216,217,218,219]:
        for x,y in [(0,0),(1018,1018),(rng.randrange(1000),rng.randrange(1000))]:
            original=bytearray([17])*1048576
            original[y*1024+x]=old
            uc.mem_write(MAP,bytes(original))
            sp=STACK+32768
            uc.mem_write(sp,struct.pack('<IIII',STOP,x,y,new))
            uc.reg_write(UC_X86_REG_ESP,sp)
            uc.emu_start(0x404240,STOP,count=10000)
            actual=(ctypes.c_uint8*1048576).from_buffer_copy(original)
            assert lib.sniper_map_place(actual,sizes,x,y,new)==0
            assert bytes(actual)==bytes(uc.mem_read(MAP,1048576)),(old,new,x,y)
            cases+=1
print(f'{cases} full-map comparisons passed against original x86 0x404240')
# Explicit safe behavior for inputs where original has unchecked memory access.
buf=(ctypes.c_uint8*1048576)()
for x,y,tile in [(1024,0,1),(0,1024,1),(1023,1023,219),(0,0,250)]:
    assert lib.sniper_map_place(buf,sizes,x,y,tile)==-1
    assert not any(buf)
print('Invalid-input guards preserve map')
for filename in ['LOCAL.LVL','45538382.LVL']:
    uc.mem_write(MAP,bytes(1048576))
    actual=(ctypes.c_uint8*1048576)()
    data=(ROOT/'code/build/assets/files'/filename).read_bytes()
    for (word,) in struct.iter_unpack('<I',data):
        x,y,tile=word&4095,(word>>12)&4095,word>>24
        sp=STACK+32768
        uc.mem_write(sp,struct.pack('<IIII',STOP,x,y,tile))
        uc.reg_write(UC_X86_REG_ESP,sp)
        uc.emu_start(0x404240,STOP,count=10000)
        assert lib.sniper_map_place(actual,sizes,x,y,tile)==0,(filename,x,y,tile)
    assert bytes(actual)==bytes(uc.mem_read(MAP,1048576)),filename
    import hashlib
    print(f'{filename}: {len(data)//4} native placements match; grid SHA256 '+hashlib.sha256(bytes(actual)).hexdigest())
