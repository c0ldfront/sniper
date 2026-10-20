"""Codec tests, including optional differential execution of original x86 code.

Run from this directory: python -m unittest -v test_compression
Original executable is never launched: Unicorn emulates isolated routines,
with allocator/free/memmove stubs. Requires optional unicorn and pefile.
"""
import random
import struct
import unittest
import hashlib
from pathlib import Path
from sniper_compression import compress, decompress

try:
    import pefile
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX
except ImportError:
    pefile = None


class Original:
    def __init__(self):
        executable = Path(__file__).resolve().parents[2] / 'build/assets/files/SNIPER.EXE'
        if hashlib.sha256(executable.read_bytes()).hexdigest() != '8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0':
            raise RuntimeError('original executable identity mismatch')
        pe = pefile.PE(str(executable))
        self.u = Uc(UC_ARCH_X86, UC_MODE_32)
        self.u.mem_map(pe.OPTIONAL_HEADER.ImageBase, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        self.u.mem_write(pe.OPTIONAL_HEADER.ImageBase, pe.get_memory_mapped_image())
        self.u.mem_map(0x1000000, 0x2000000)
        self.u.mem_map(0x4000000, 0x100000)
        self.next = 0x1000000
        self.allocs = {}
        self.capacity = {}
        self.u.hook_add(UC_HOOK_CODE, self.hook)

    def alloc(self, size):
        p = self.next
        capacity = (max(size, 1) + 4095) & ~4095
        self.next += capacity
        if self.next >= 0x3000000:
            raise RuntimeError('emulator heap limit')
        self.allocs[p] = size
        self.capacity[p] = capacity
        return p

    def hook(self, u, address, size, user):
        if address not in (0x416190, 0x416210, 0x416b20, 0x416170, 0x416620, 0x416280):
            return
        sp = u.reg_read(UC_X86_REG_ESP)
        ret, a, b, c = struct.unpack('<4I', bytes(u.mem_read(sp, 16)))
        result = 0
        if address in (0x416190, 0x416b20):
            result = self.alloc(a)
        elif address == 0x416210:
            if a and b <= self.capacity[a]:
                result = a
                self.allocs[a] = b
            else:
                result = self.alloc(b)
                if a:
                    u.mem_write(result, bytes(u.mem_read(a, min(self.allocs[a], b))))
        elif address == 0x416280:
            if c:
                u.mem_write(a, bytes(u.mem_read(b, c)))
            result = a
        u.reg_write(UC_X86_REG_EAX, result)
        u.reg_write(UC_X86_REG_ESP, sp + 4)
        u.reg_write(UC_X86_REG_EIP, ret)

    def call(self, address, args):
        sp = 0x40f0000
        stop = 0x4000000
        self.u.mem_write(sp, struct.pack('<' + 'I' * (len(args) + 1), stop, *args))
        self.u.reg_write(UC_X86_REG_ESP, sp)
        self.u.emu_start(address, stop, count=10000000)
        if self.u.reg_read(UC_X86_REG_EIP) != stop:
            raise RuntimeError('original routine instruction budget exceeded')
        return self.u.reg_read(UC_X86_REG_EAX)

    def encode(self, data, passes=1, disable=0):
        ptr, out = self.alloc(len(data)), self.alloc(4)
        self.u.mem_write(ptr, data)
        result = self.call(0x410920, [ptr, len(data), out, disable, passes])
        size = struct.unpack('<I', self.u.mem_read(out, 4))[0]
        return bytes(self.u.mem_read(result, size))

    def decode(self, data):
        ptr, out = self.alloc(len(data)), self.alloc(4)
        self.u.mem_write(ptr, data)
        # Decompiled unused parameters are register artifacts: actual cdecl
        # argument layout is checked against routine entry instructions.
        result = self.call(0x410c60, [ptr, len(data), out])
        size = struct.unpack('<I', self.u.mem_read(out, 4))[0]
        return bytes(self.u.mem_read(result, size))


class CodecTests(unittest.TestCase):
    def test_roundtrip(self):
        rng = random.Random(87)
        for data in (b'', b'A', b'A' * 4096, bytes(range(256)), b'AB' * 4096,
                     b'ABCD' * 3000, bytes(rng.randrange(8) for _ in range(10000))):
            for passes in (0, 1, 3):
                self.assertEqual(decompress(compress(data, passes)), data)

    def test_malformed(self):
        for bad in (b'', b'\0\0\0', b'\0\0\0\1', b'\xff' * 4):
            with self.assertRaises(ValueError):
                decompress(bad)
        valid = compress(b'AB' * 4096)
        with self.assertRaises(ValueError):
            decompress(valid[:-1])
        with self.assertRaises(ValueError):
            decompress(valid, max_output=100)

    @unittest.skipUnless(pefile, 'optional unicorn/pefile unavailable')
    def test_original_differential(self):
        rng = random.Random(123)
        cases = [b'hello', bytes(range(256)), b'AB' * 4096, b'ABCD' * 3000, b'ABA' * 2001,
                 bytes(rng.randrange(8) for _ in range(10000))]
        for data in cases:
            for passes in (0, 1, 3):
                with self.subTest(length=len(data), passes=passes):
                    expected = compress(data, passes)
                    original = Original().encode(data, passes)
                    self.assertEqual(original, expected)
                    self.assertEqual(Original().decode(expected), data)
                    self.assertEqual(decompress(original), data)

    @unittest.skipUnless(pefile, 'optional unicorn/pefile unavailable')
    def test_original_single_symbol_bug(self):
        original = Original().encode(b'A' * 4096)
        self.assertEqual(original[:4], b'\0\0\0\1')
        self.assertEqual(Original().decode(original), b'')
        with self.assertRaises(ValueError):
            decompress(original)
        self.assertEqual(compress(b'A' * 4096)[:4], b'\0' * 4)


if __name__ == '__main__':
    unittest.main()
