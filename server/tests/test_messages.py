import unittest
from messages import encode, decode, pack_state_flags, unpack_state_flags

class MessagesTests(unittest.TestCase):
    def test_join_golden(self):
        p=encode('c2s',1,ship_type=2,team=-1,option=1,screen_width=640,screen_height=480)
        self.assertEqual(p.hex(),'0102ffffffff0100000080020000e0010000')
        self.assertEqual(decode('c2s',p)['fields']['team'],-1)
    def test_direction_difference(self):
        self.assertEqual(encode('c2s',16,team=7),bytes.fromhex('1007000000'))
        self.assertEqual(encode('s2c',16,player=7,target=-1),bytes.fromhex('100700ffff'))
    def test_chat_golden(self):
        self.assertEqual(encode('c2s',6,target=-1,text=b'hi'),bytes.fromhex('06ffff686900'))
        self.assertEqual(encode('s2c',7,sender=3,target=-1,text=b'hi'),bytes.fromhex('0703ffff686900'))
    def test_state_golden(self):
        f=dict(state_byte=1,word2=2,word4=-3,word6=4,word8=-5,tick=0x12345678,extra_byte=6,extra_word=-7,flags=0x8123)
        p=encode('c2s',3,**f)
        self.assertEqual(p.hex(),'03010200fdff0400fbff7856341206f9ff2381')
        self.assertEqual(decode('c2s',p)['fields'],f)
        f['player']=9
        self.assertEqual(encode('s2c',5,**f).hex(),'05010200fdff0400fbff097856341206f9ff2381')
    def test_login_and_types(self):
        p=encode('c2s',9,create_user=0,name=b'pilot',password=b'secret',third_string=b'')
        self.assertEqual(len(p),98)
        self.assertEqual(p[2:34],b'pilot'+bytes(27))
        self.assertEqual(p[34:66],b'secret'+bytes(26))
        self.assertEqual(p[66:],bytes(32))
        with self.assertRaises(ValueError):
            encode('c2s',9,create_user=0,name=b'x'*33,password=b'',third_string=b'')
    def test_file_payload_not_executed(self):
        p=encode('s2c',18,filename=b'../x',data=b'abc')
        self.assertEqual(decode('s2c',p)['fields']['data'],b'abc')
    def test_list(self):
        p=bytes.fromhex('0b610062000100020003000000')
        self.assertEqual(decode('s2c',p)['fields']['records'][0]['value3'],3)
        self.assertEqual(encode('s2c',11,records=[dict(string1=b'a',string2=b'b',value1=1,value2=2,value3=3)]),p)
        for bad in [p[:-1],b'\x0babc']:
            with self.assertRaises(ValueError): decode('s2c',bad)
    def test_reject_malformed(self):
        for direction,p in [('s2c',b''),('s2c',b'\x01'),('s2c',b'\x01\x02\x03'),('c2s',b'\x06\xff\xffabc'),('s2c',b'\xff'),('c2s',b'\x06\xff\xffa\0b\0')]:
            with self.assertRaises(ValueError): decode(direction,p)
    def test_fixed_string_and_opaque_bounds(self):
        for filename in (b'x'*16,b'x'*17):
            with self.assertRaises(ValueError): encode('s2c',18,filename=filename,data=b'')
        self.assertEqual(len(encode('s2c',18,filename=b'x'*15,data=b'')),17)
        self.assertEqual(len(encode('s2c',18,filename=b'x'*15+b'\0',data=b'')),17)
        for n in (0,287,289):
            with self.assertRaises(ValueError): encode('s2c',17,opaque=bytes(n))
        self.assertEqual(len(encode('s2c',17,opaque=bytes(288))),289)

    def test_flags(self):
        self.assertEqual(pack_state_flags(7,3,7,3,31,1),65535)
        self.assertEqual(unpack_state_flags(0x8001),(1,0,0,0,0,1))
        with self.assertRaises(ValueError): pack_state_flags(8,0,0,0,0,0)

class OriginalSenderTests(unittest.TestCase):
    def capture(self, address, args, text=b''):
        try:
            import unicorn as u
            from unicorn import x86_const as x
        except ImportError:
            self.skipTest('optional Unicorn dependency unavailable')
        import struct
        from pathlib import Path
        binary=(Path(__file__).resolve().parents[2]/'build'/'assets'/'files'/'SNIPER.EXE').read_bytes()
        import hashlib
        self.assertEqual(hashlib.sha256(binary).hexdigest(),'8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0')
        cpu=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
        cpu.mem_map(0x400000,0x100000)
        cpu.mem_map(0x600000,0x1000)
        cpu.mem_map(0x700000,0x100000)
        pe=struct.unpack_from('<I',binary,60)[0]
        count=struct.unpack_from('<H',binary,pe+6)[0]
        first=pe+24+struct.unpack_from('<H',binary,pe+20)[0]
        for i in range(count):
            virtual_size,rva,size,offset=struct.unpack_from('<IIII',binary,first+40*i+8)
            if size and rva < 0x100000:
                amount=min(size,0x100000-rva)
                cpu.mem_write(0x400000+rva,binary[offset:offset+amount])
        cpu.mem_write(0x4353f4,struct.pack('<I',0x600000))
        cpu.mem_write(0x600000,b'\xc3')
        cpu.mem_write(0x720010,struct.pack('<I',0x730000))
        cpu.mem_write(0x7308ac,struct.pack('<I',5))
        cpu.mem_write(0x740000,text+b'\0')
        cpu.mem_write(0x710000,struct.pack('<'+'I'*(len(args)+1),0x600001,*[a&0xffffffff for a in args]))
        cpu.reg_write(x.UC_X86_REG_ESP,0x710000)
        cpu.reg_write(x.UC_X86_REG_ECX,0x720000)
        captured=[]
        def hook(cpu,pc,size,data):
            if pc==0x600000:
                cpu.reg_write(x.UC_X86_REG_EAX,5000)
            elif pc==0x401610:
                esp=cpu.reg_read(x.UC_X86_REG_ESP)
                pointer,length=struct.unpack('<II',cpu.mem_read(esp+4,8))
                captured.append(bytes(cpu.mem_read(pointer,length)))
                cpu.emu_stop()
        cpu.hook_add(u.UC_HOOK_CODE,hook)
        cpu.emu_start(address,0x600001,count=10000)
        self.assertEqual(len(captured),1)
        return captured[0]

    def test_original_senders(self):
        cases=[
            (0x411970,[4,123],encode('c2s',5,other_player=4,value=123)),
            (0x411b10,[7],encode('c2s',8,action=7)),
            (0x411c20,[-2],encode('c2s',16,team=-2)),
            (0x411cc0,[-1],encode('c2s',17,target=-1)),
        ]
        for address,args,expected in cases:
            with self.subTest(address=hex(address)):
                self.assertEqual(self.capture(address,args),expected)
        self.assertEqual(self.capture(0x411a50,[3,0x740000],b'hello'),encode('c2s',6,target=3,text=b'hello'))
        expected=encode('c2s',3,state_byte=9,word2=11,word4=-12,word6=13,word8=-14,tick=505,extra_byte=8,extra_word=-15,flags=pack_state_flags(1,2,3,1,17,1))
        self.assertEqual(self.capture(0x411860,[11,-12,13,-14,9,1,2,3,1,17,1,8,-15]),expected)

if __name__=='__main__': unittest.main()

