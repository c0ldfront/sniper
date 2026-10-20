"""Verify source-linked original-address functions and persistent RNG state."""
from pathlib import Path
import hashlib, json, re, struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'research/experiments/exact-gameplay-parity/link-layout'
original = ROOT/'code/build/assets/files/SNIPER.EXE'
built = OUT/'build/core.dll'
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(original) == '8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0'
a, b = pefile.PE(str(original)), pefile.PE(str(built))
assert a.OPTIONAL_HEADER.ImageBase == b.OPTIONAL_HEADER.ImageBase == 0x400000
mapping = (OUT/'build/core.map').read_text()
checks = []
for symbol, address, length, section in [('?reset@Clock@@QAEHXZ',0x401430,16,'.text$b'),('?step@Pickup@@QAEHH@Z',0x412b00,28,'.text$d'),('_core_rand',0x4165f0,42,'.text$f')]:
    match = re.search(re.escape(symbol)+r'\s+([0-9a-fA-F]{8})\s+f', mapping)
    assert match and int(match.group(1),16)==address, symbol
    contribution = re.search(r'\s([0-9a-fA-F]+)H\s+'+re.escape(section)+r'\s+CODE',mapping)
    assert contribution and int(contribution.group(1),16)==length, symbol
    expected, actual = a.get_data(address-0x400000,length), b.get_data(address-0x400000,length)
    assert len(expected)==len(actual)==length and actual==expected, symbol
    checks.append(dict(symbol=symbol,address=hex(address),length=length,raw_match=True,bytes_hex=actual.hex()))
assert a.get_data(0x22f78,4)==b.get_data(0x22f78,4)==b'\x01\x00\x00\x00'
state_mapping = re.search(r'_core_rng_state\s+([0-9a-fA-F]{8})',mapping)
assert state_mapping and int(state_mapping.group(1),16)==0x422f78
engines=[]
for pe in [a,b]:
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
    u.mem_write(0x400000,pe.get_memory_mapped_image());engines.append(u)
trace=hashlib.sha256();count=0
for seed in [0,1,87,0x7fffffff,0x80000000,0xffffffff,0x12345678,0xdeadbeef]:
    for u in engines:u.mem_write(0x422f78,struct.pack('<I',seed))
    for draw in range(1024):
        values=[]
        for u in engines:
            u.mem_write(0x800000,struct.pack('<I',0x810000));u.reg_write(UC_X86_REG_ESP,0x800000)
            u.emu_start(0x4165f0,0x810000,count=100)
            assert u.reg_read(UC_X86_REG_EIP)==0x810000
            values.append((u.reg_read(UC_X86_REG_EAX),bytes(u.mem_read(0x422f78,4))))
        assert values[0]==values[1],(seed,draw,values)
        trace.update(struct.pack('<I',values[0][0])+values[0][1]);count+=1
inputs=[original,built,OUT/'core.cpp',OUT/'layout.s',OUT/'build.sh',Path(__file__),OUT/'build/core.map']
inputs += [ROOT.parent/'tools/legacy-msvc/MSVC400/BIN'/name for name in ['CL.EXE','C1XX.EXE','C2.EXE','LINK.EXE']]
result=dict(status='PASS_SUBSET_ONLY',checks=checks,matched_instruction_bytes=86,rng_state_address='0x422f78',initial_state=1,persistent_rng_draws=count,trace_sha256=trace.hexdigest(),files={str(p.relative_to(ROOT)):sha(p) for p in inputs},limits=['Only three complete functions; scope inventory unresolved.','Source compiled by VC4 and placed by LINK3 with layout-only COFF padding; no code bytes copied or post-link patched.','Entire core is NOT reconstructed or integrated into the client; unimplemented gaps contain INT3.','RNG actual linked state and function are compared; this is not a full gameplay claim.'])
(OUT/'result.json').write_text(json.dumps(result,indent=2)+'\n')
print(f'PASS subset:3 functions/86 literal linked instruction bytes; {count} persistent RNG draws; overall exact-gameplay goal remains incomplete')
