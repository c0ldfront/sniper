"""Validate every atlas frame against actual PE bitmap dimensions and callback wiring."""
import ctypes as C
import pathlib,struct
import pefile
ROOT=pathlib.Path(__file__).resolve().parents[2]
lib=C.CDLL(str(ROOT/'code/build/librender_commands.so'))
class Sprite(C.Structure):_fields_=[(n,C.c_int32) for n in ['resource','origin_x','origin_y','width','height','rows','columns','period','frames']]
class Rect(C.Structure):_fields_=[(n,C.c_int32) for n in ['left','top','right','bottom']]
class Blit(C.Structure):_fields_=[('source',Rect),('destination',Rect),('flags',C.c_uint32)]
class Command(C.Structure):_fields_=[('resource',C.c_int),('blit',Blit)]
Submit=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(Command))
class Sink(C.Structure):_fields_=[('submit',Submit),('user',C.c_void_p),('failed',C.c_int)]
lib.sniper_resource_command.argtypes=[C.POINTER(Sprite),C.c_int32,C.c_int32,C.c_int32,C.c_int,C.c_int,C.POINTER(Command)]
p=pefile.PE(str(ROOT/'code/build/assets/files/SNIPER.EXE'));dimensions={}
for t in p.DIRECTORY_ENTRY_RESOURCE.entries:
    if t.id!=2:continue
    for item in t.directory.entries:
        d=item.directory.entries[0].data.struct
        data=p.get_data(d.OffsetToData,d.Size)
        dimensions[item.id]=struct.unpack_from('<ii',data,4)
ships=(Sprite*9).in_dll(lib,'sniper_ship_assets');effects=(Sprite*2).in_dll(lib,'sniper_effect_assets');fonts=(Sprite*4).in_dll(lib,'sniper_fonts')
count=0
for s in [*ships,*effects,*fonts]:
    for frame in range(s.frames or s.rows*s.columns):
        command=Command()
        assert lib.sniper_resource_command(C.byref(s),100,200,frame,1,0,C.byref(command))==0
        r=command.blit.source;w,h=dimensions[command.resource]
        assert 0<=r.left<r.right<=w and 0<=r.top<r.bottom<=abs(h),(s.resource,frame,tuple(getattr(r,n) for n,_ in Rect._fields_),w,h)
        assert r.right-r.left==s.width and r.bottom-r.top==s.height
        assert command.blit.flags==0x8000
        count+=1
seen=[]
@Submit
def submit(_,cmd):seen.append(bytes(cmd.contents))
sink=Sink(submit,None,0)
lib.sniper_command_body.argtypes=[C.c_void_p,C.c_int,C.c_int32,C.c_int32,C.c_int32]
lib.sniper_command_text.argtypes=[C.c_void_p,C.c_char_p,C.c_int32,C.c_int32,C.c_int]
lib.sniper_command_body(C.byref(sink),3,100,200,0)
c=Command.from_buffer_copy(seen[-1]);assert c.resource==700 and c.blit.source.top==384
lib.sniper_command_text(C.byref(sink),b'AB',10,20,2)
a,b=map(Command.from_buffer_copy,seen[-2:]);assert a.resource==b.resource==139
assert a.blit.destination.left==10 and b.blit.destination.left==16
assert a.blit.source.top>=32 and b.blit.source.top>=32 and sink.failed==0
print(f'{count} atlas frames fit original bitmap resources; ship crop and font callback wiring pass')
