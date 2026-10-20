"""Production window/menu/display/session against loopback-only game peer."""
import json,zlib,io,wave,hashlib
import pathlib,socket,struct,threading,subprocess,time,os,ctypes as C
root=pathlib.Path(__file__).resolve().parents[2];run=root/'research/sniper87/runtime-online-startup';run.mkdir(exist_ok=True)
exe=root/'code/platform/build/online-startup-test.exe';assert exe.exists()
subprocess.run(['cc','-shared','-fPIC',str(root/'code/src/local_config.c'),'-o',str(root/'code/build/libonline_fixture_config.so')],check=True)
config=(C.c_int32*72)();C.CDLL(str(root/'code/build/libonline_fixture_config.so')).sniper_local_config(C.byref(config));config[70]=1;settings=b'\x11'+bytes(config)
def checksum(b):return sum(struct.unpack('<'+'I'*(len(b)//4),b[:len(b)//4*4]))&0xffffffff
tcp=socket.socket();tcp.bind(('127.0.0.1',0));tcp.listen();tcp.settimeout(90);udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);udp.bind(('127.0.0.1',0));udp.settimeout(30)
level=(root/'code/build/assets/files/LOCAL.LVL').read_bytes();arena=b'Local,127.0.0.1,382\n';name=('127001'+str(tcp.getsockname()[1])+'.lvl')[-12:];(run/name).write_bytes(level);(run/'arena.lst').write_bytes(arena)
recordings=[]
for slot in range(2):
    buffer=io.BytesIO()
    with wave.open(buffer,'wb') as wav:
        wav.setnchannels(1);wav.setsampwidth(1);wav.setframerate(8000);wav.writeframes(bytes((i*37+slot*53)%256 for i in range(96)))
    recordings.append(buffer.getvalue());(run/f'record{slot}.wav').write_bytes(recordings[-1])
subprocess.run(['cc','-shared','-fPIC',str(root/'code/src/network_compression.c'),'-o',str(root/'code/build/libonline_fixture_codec.so')],check=True)
codec=C.CDLL(str(root/'code/build/libonline_fixture_codec.so'));codec.sniper_network_decompress.argtypes=[C.c_void_p,C.c_int32,C.POINTER(C.c_int32)];codec.sniper_network_decompress.restype=C.c_void_p
libc=C.CDLL(None);libc.free.argtypes=[C.c_void_p]
def decoded(data):
    output_size=C.c_int32();output=codec.sniper_network_decompress(data,len(data),C.byref(output_size));assert output
    result=C.string_at(output,output_size.value);libc.free(output);return result
errors=[];seen=[];bye=[];voice_cache={};voice_wire=[]
file_payload=b"Sniper87 loopback opcode18 artifact\n"
blocked_name="../wirebad.txt"
assert not (run.parent/"wirebad.txt").exists()
def peer():
 try:
    conn,_=tcp.accept();conn.settimeout(15);hello=b''
    while len(hello)<6:hello+=conn.recv(6-len(hello))
    addr=(socket.inet_ntoa(hello[:4]),struct.unpack('!H',hello[4:])[0]);conn.sendall(socket.inet_aton('127.0.0.1')+struct.pack('!H',udp.getsockname()[1]));assert conn.recv(1)==b'';conn.close();sequence=1000;dedup=set()
    def reliable(body):
        nonlocal sequence
        udp.sendto(b'\0\2'+struct.pack('<IH',sequence,(sequence-1000)%256)+body,addr);sequence+=1
    while len(bye)<4:
        data,source=udp.recvfrom(4096);assert source==addr
        if data[:2]==b'\0\0':udp.sendto(b'\0\1'+data[2:6]+struct.pack('<I',int(time.monotonic()*100)%0x19999999),addr);continue
        if data[:2]==b'\0\3':continue
        if data==b'\0\4':bye.append(time.monotonic());continue
        if data[:2]==b'\0\2':
            seq=struct.unpack('<I',data[2:6])[0];udp.sendto(b'\0\3'+data[2:8],addr)
            if seq in dedup:continue
            dedup.add(seq);data=data[8:]
        op=data[0];seen.append(op)
        if op==9:
            assert data[2:11]==b'Online42\0';reliable(b'\x0a\0'+struct.pack('<iIII',87,sum(level[:len(level)//4*4])&0xffffffff,checksum(exe.read_bytes()),checksum(arena)))
        elif op==1:
            assert len(data)==18
            reliable(settings);reliable(b'\1\x2a');join=bytearray(85);join[0]=3;join[1]=42;join[3:12]=b'Online42\0';join[79:81]=b'\xff\xff';join[81:85]=struct.pack('<I',1);reliable(join);join[1]=7;join[3:12]=b'Remote07\0';reliable(join);reliable(b'\2');pos=bytearray(20);pos[0]=5;pos[10]=7;pos[2:6]=struct.pack('<hh',2000,2000);pos[11:15]=struct.pack('<I',int(time.monotonic()*100)%0x19999999);reliable(pos)
            reliable(bytes([18])+b"WIRETEST.TXT".ljust(16,b"\0")+file_payload)
            reliable(bytes([18])+blocked_name.encode().ljust(16,b"\0")+b"must not write")
        elif op==15:
            target,slot=struct.unpack('<hB',data[1:4]);assert target==7 and slot in (0,1)
            first=slot not in voice_cache
            if first:
                assert len(data)>4;voice_cache[slot]=data[4:];assert decoded(voice_cache[slot])==recordings[slot]
            else:assert len(data)==4
            voice_wire.append(dict(slot=slot,first=first,length=len(data)))
            reliable(bytes([14,7])+voice_cache[slot])
        elif op in (2,3,5,7,16,17):pass
        else:raise AssertionError((op,data))
 except BaseException as e:errors.append(e)
t=threading.Thread(target=peer,daemon=True);t.start();env=os.environ.copy();env.update(WINEPREFIX=str(run/'wine'),WINEDEBUG='-all',LIBGL_ALWAYS_SOFTWARE='1',__GLX_VENDOR_LIBRARY_NAME='mesa',__EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json')
alsa=run/'alsa-null.conf';alsa.write_text('pcm.!default { type null }\nctl.!default { type hw card 0 }\n');env['ALSA_CONFIG_PATH']=str(alsa)
subprocess.run(['wine','reg','add',r'HKCU\Software\Wine\Drivers','/v','Audio','/d','alsa','/f'],env=env,capture_output=True,timeout=30,check=True)
try:result=subprocess.run(['xvfb-run','-a','-s','-screen 0 640x480x24','wine',str(exe),str(tcp.getsockname()[1])],cwd=run,env=env,capture_output=True,text=True,timeout=100)
except subprocess.TimeoutExpired as e:
 (run/'result.txt').write_bytes(e.stdout or b'');(run/'wine.log').write_bytes(e.stderr or b'');raise
finally:subprocess.run(['wineserver','-k'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
(run/'result.txt').write_text(result.stdout);(run/'wine.log').write_text(result.stderr);t.join(timeout=2);tcp.close();udp.close()
assert result.returncode==0,(result.returncode,result.stdout,result.stderr,errors)
assert not errors and not t.is_alive(),errors
assert seen[0:2]==[9,1] and 3 in seen and 5 in seen and seen[-1]==2,seen
print(result.stdout.strip());print('PASS server wire authentication/join,position/death/gameleave and four disconnect notifications')

assert (run/"WIRETEST.TXT").read_bytes()==file_payload
meta=json.loads((run/"WIRETEST.TXT.source.json").read_text())
assert meta["hostname"]=="127.0.0.1" and meta["bytes"]==len(file_payload)
assert meta["actual_checksum"]==f"{zlib.crc32(file_payload):08x}"
assert not (run.parent/"wirebad.txt").exists()
print("PASS production opcode18 artifact/checksum/provenance and traversal rejection")

assert [(entry['slot'],entry['first']) for entry in voice_wire]==[(0,True),(0,False),(1,True),(1,False)],voice_wire
(run/'voice-wire-result.json').write_text(json.dumps({'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'wire':voice_wire,'recording_sha256':[hashlib.sha256(x).hexdigest() for x in recordings]},indent=2)+'\n')
print('PASS actual ShiftF11/F12 first payload+cached repeat,peer decode and incoming voice roundtrip')
