"""Independent original-instruction oracle for the complete settings baseline."""
import hashlib
import json
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parent

class OriginalSettingsTests(unittest.TestCase):
    def test_all_settings_match_original_x86_initialization(self):
        try:
            import unicorn as u
            from unicorn import x86_const as x
        except ImportError:
            self.skipTest('Unicorn is required for original-instruction validation')
        from server import GAME_FILES, offline_settings
        binary = (GAME_FILES/'SNIPER.EXE').read_bytes()
        self.assertEqual(hashlib.sha256(binary).hexdigest(),
                         '8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0')
        cpu = u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
        cpu.mem_map(0x400000,0x100000)
        cpu.mem_map(0x700000,0x1000)
        cpu.reg_write(x.UC_X86_REG_EBP,0x700100)
        pe = struct.unpack_from('<I',binary,60)[0]
        count = struct.unpack_from('<H',binary,pe+6)[0]
        first = pe+24+struct.unpack_from('<H',binary,pe+20)[0]
        for index in range(count):
            _,rva,size,offset = struct.unpack_from('<IIII',binary,first+40*index+8)
            if size and rva<0x100000:
                amount = min(size,0x100000-rva)
                cpu.mem_write(0x400000+rva,binary[offset:offset+amount])
        cpu.mem_write(0x4281c9,b'\xcc'*288)
        cpu.emu_start(0x4110ab,0x411335,count=1000)
        self.assertEqual(cpu.reg_read(x.UC_X86_REG_EIP),0x411335)
        actual = bytes(cpu.mem_read(0x4281c9,288))
        expected = offline_settings()  # the table in src/local_config.c
        self.assertEqual(actual,expected)
        self.assertEqual(len(actual),288)

if __name__=='__main__':
    unittest.main()
