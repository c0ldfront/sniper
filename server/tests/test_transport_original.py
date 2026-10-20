"""Isolated x86 sender oracle; never launches EXE or uses Winsock/network."""
import struct
import unittest
from test_compression import Original, pefile
from transport import encode_control

if pefile:
    from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP


class SenderOriginal(Original):
    def __init__(self):
        super().__init__()
        self.u.mem_write(0x4353f4, struct.pack('<I', 0x4000100))

    def hook(self, u, address, size, user):
        if address in (0x4000100, 0x401e00):
            sp = u.reg_read(UC_X86_REG_ESP)
            ret, = struct.unpack('<I', u.mem_read(sp, 4))
            u.reg_write(UC_X86_REG_EAX, 123450 if address == 0x4000100 else 0)
            u.reg_write(UC_X86_REG_ESP, sp + 4)
            u.reg_write(UC_X86_REG_EIP, ret)
        else:
            super().hook(u, address, size, user)

    def send_reliable(self, payload, sequence, head, count):
        obj, data = self.alloc(0x3c58), self.alloc(len(payload))
        self.u.mem_write(data, payload)
        for offset, value in [(0xc, 1), (0x10, 10), (0x8b0, 123),
                              (0x1bc8, sequence), (0x1bcc, head), (0x1bd0, count)]:
            self.u.mem_write(obj + offset, struct.pack('<I', value))
        self.u.reg_write(UC_X86_REG_ECX, obj)
        status = self.call(0x401610, [data, len(payload), 1, 0])
        slot = (head + count) % 256
        seq, ptr, length, tick = struct.unpack('<4I', self.u.mem_read(obj + 0xbc8 + slot*16, 16))
        next_seq, = struct.unpack('<I', self.u.mem_read(obj + 0x1bc8, 4))
        return status & 255, seq, bytes(self.u.mem_read(ptr, length)), tick, next_seq


@unittest.skipUnless(pefile, 'optional unicorn/pefile unavailable')
class OriginalSenderTests(unittest.TestCase):
    def test_real_sender_envelopes(self):
        for payload in (b'\x19', b'\x19hello', b'\0\5'+b'A'*480, b'\0\5', b'X'*500):
            for sequence, head, count in [(1000,0,0),(12345,250,10),(0xffffffff,255,0)]:
                with self.subTest(size=len(payload), sequence=sequence, head=head):
                    status, seq, data, tick, next_seq = SenderOriginal().send_reliable(payload, sequence, head, count)
                    self.assertEqual(status, 1)
                    self.assertEqual(seq, sequence)
                    self.assertEqual(data, encode_control(2, sequence=sequence, slot=(head+count)%256, payload=payload))
                    self.assertEqual(tick, (12345-100000)&0xffffffff)
                    self.assertEqual(next_seq, (sequence+1)&0xffffffff)

if __name__ == '__main__':
    unittest.main()
