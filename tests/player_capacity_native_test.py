"""Probe native static player-array extents and unchecked insertion boundary."""
import pathlib
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ESI

root = pathlib.Path(__file__).resolve().parents[2]
u = Uc(UC_ARCH_X86, UC_MODE_32)
u.mem_map(0x400000, 0x200000)
u.mem_write(0x400000, pefile.PE(str(root / 'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(address, value):
    u.mem_write(address, struct.pack('<I', value))
def get(address):
    return struct.unpack('<I', u.mem_read(address, 4))[0]

# Original initialization: sentinel immediately after each allocation survives.
u.mem_write(0x4283e8, b'\xa5' * (257 * 4))
u.mem_write(0x429338, b'\xa5' * (256 * 4))
u.reg_write(UC_X86_REG_EAX, 0)
u.reg_write(UC_X86_REG_ESI, 0)
u.emu_start(0x402987, 0x4029c9)
assert bytes(u.mem_read(0x4283e8, 256 * 4)) == bytes(256 * 4)
assert get(0x4287e8) == 0xa5a5a5a5
assert bytes(u.mem_read(0x429338, 255 * 4)) == bytes(255 * 4)
assert get(0x429734) == 0xa5a5a5a5

# Registration's actual store sequence has neither ID nor count guards.
for player_id, count in [(0, 0), (254, 254), (255, 255), (0, 256)]:
    put(0x429328, count)
    put(0x429734, 0x12345678)
    put(0x4287e8, 0x87654321)
    u.reg_write(UC_X86_REG_EAX, 0x510000)
    u.reg_write(UC_X86_REG_EBX, player_id)
    u.emu_start(0x4043be, 0x4043d8)
    assert get(0x429338 + player_id * 4) == 0x510000
    assert get(0x4283e8 + count * 4) == 0x510000
    assert get(0x429328) == count + 1
    assert get(0x429734) == (0x510000 if player_id == 255 else 0x12345678)
    assert get(0x4287e8) == (0x510000 if count == 256 else 0x87654321)
print('PASS native table extents: indexed255, ordered256; unchecked ID255/count256 overwrite adjacent globals')
