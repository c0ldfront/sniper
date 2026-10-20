"""Native entry reset block and full teardown: chat process/session lifetimes.
Resource/audio/OS cleanup is stubbed; no reconstructed implementation oracle.
"""
import pathlib,random,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP
root=pathlib.Path(__file__).resolve().parents[2]
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x500000)
u.mem_write(0x400000,pefile.PE(str(root/'code/build/assets/files/SNIPER.EXE')).get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def get(a):return struct.unpack('<i',u.mem_read(a,4))[0]
released=[]
def hook(uc,a,size,user):
    if a not in (0x510000,0x510010,0x40a8c0,0x40a650):return
    sp=uc.reg_read(UC_X86_REG_ESP);extra=0
    if a==0x510010:released.append(get(sp+4));extra=4
    uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+extra)
u.hook_add(UC_HOOK_CODE,hook)
r=random.Random(87)
for case in range(100):
    u.mem_write(0x424000,bytes(0x10000));put(0x43556c,0x510000)
    rows=r.choice([-2,0,2,3,4,16,17,100]);spectator=case%2
    blocks={0x428358:r.randbytes(128),0x428a78:r.randbytes(2048),0x428a10:r.randbytes(64),0x42e2a8:r.randbytes(656)}
    for a,b in blocks.items():u.mem_write(a,b)
    ticker=r.randrange(33);tick=r.randrange(-2147483648,2147483648);dirty=r.randrange(-2,3)
    for a,v in [(0x42227c,rows),(0x421a30,ticker),(0x428a74,tick),(0x428188,dirty),(0x428a60,0x520000),(0x810008,spectator)]:put(a,v)
    u.reg_write(UC_X86_REG_ESP,0x800000);u.reg_write(UC_X86_REG_EBP,0x810000)
    u.emu_start(0x402709,0x402913,count=10000)
    assert get(0x42227c)==min(rows,16)
    assert get(0x42819c)==(0 if spectator else 1)
    assert get(0x428a60)==0
    for a,n in [(0x428358,128),(0x428a78,2048),(0x428a10,64)]:assert bytes(u.mem_read(a,n))==bytes(n)
    assert bytes(u.mem_read(0x42e2a8,656))==blocks[0x42e2a8]
    assert (get(0x421a30),get(0x428a74),get(0x428188))==(ticker,tick,dirty)
    # Populate active session chat state before full teardown. No other owned
    # resources in this fixture; destructor should only release our chat cache.
    for a,b in blocks.items():u.mem_write(a,b)
    put(0x428a60,0x520000);put(0x520000,0x520100);put(0x520108,0x510010)
    put(0x800000,0x800100);u.reg_write(UC_X86_REG_ESP,0x800000);released.clear()
    u.emu_start(0x403880,0x800100,count=10000)
    assert released==[0x520000]
    for a,b in blocks.items():assert bytes(u.mem_read(a,len(b)))==b
    assert (get(0x421a30),get(0x428a74),get(0x428188))==(ticker,tick,dirty)
    # Original Release leaves stale pointer; next entry nulls it.
    assert get(0x428a60)==0x520000
print('PASS100 native chat lifecycle pairs: entry clears input/history/colors, retains macros/ticker/build tick; teardown releases cache and preserves data')
