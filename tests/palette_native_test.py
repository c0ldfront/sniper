"""Original412150 resource branch; capture CreatePalette's actual entries."""
import ctypes as C
import pathlib,struct
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
ROOT=pathlib.Path(__file__).resolve().parents[2];p=pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'))
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,0x200000);u.mem_write(0x400000,p.get_memory_mapped_image())
def put(a,v):u.mem_write(a,struct.pack('<I',v))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
handlers={};next_addr=0x530000
for dll in p.DIRECTORY_ENTRY_IMPORT:
    for imp in dll.imports:
        name=(imp.name or b'').decode()
        if name in ['FindResourceA','LoadResource','LockResource']:
            handlers[next_addr]=name;put(imp.address,next_addr);next_addr+=16
put(0x500000,0x500100);put(0x500114,0x540000)
seen=[]
def hook(uc,a,size,data):
    if a not in handlers and a!=0x540000:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    if a==0x540000:
        assert get(sp+8)==4
        seen.append(bytes(uc.mem_read(get(sp+12),1024)))
        put(get(sp+16),0x550000);result=0;clean=20
    else:
        name=handlers[a];clean={'FindResourceA':12,'LoadResource':8,'LockResource':4}[name]
        result=0x560000 if name=='LockResource' else 1
    uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_EIP,get(sp));uc.reg_write(UC_X86_REG_ESP,sp+4+clean)
u.hook_add(UC_HOOK_CODE,hook)
lib=C.CDLL(str(ROOT/'code/build/libpalette.so'));lib.sniper_palette_resource.argtypes=[C.c_void_p,C.c_void_p,C.c_size_t]
fixtures=[None]
for t in p.DIRECTORY_ENTRY_RESOURCE.entries:
    if t.id==2:
        for entry in t.directory.entries:
            d=entry.directory.entries[0].data.struct;fixtures.append(p.get_data(d.OffsetToData,d.Size))
for data in fixtures:
    if data:u.mem_write(0x560000,data)
    for i,v in enumerate([0x520000,0x500000,107 if data else 0]):put(0x510000+4*i,v)
    u.reg_write(UC_X86_REG_ESP,0x510000);seen.clear()
    u.emu_start(0x412150,0x520000,count=50000)
    out=C.create_string_buffer(1024);buf=C.create_string_buffer(data) if data else None
    assert lib.sniper_palette_resource(out,buf,len(data) if data else 0)==0
    assert seen==[out.raw]
print(f'{len(fixtures)} palette cases match all256 original entries and CreatePalette flags')
