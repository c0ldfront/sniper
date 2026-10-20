"""Actual Win32 production connection controller against loopback-only peer.
Own Xvfb and Wine prefix; no shared display/game-prefix interaction.
"""
import json
import pathlib,socket,struct,threading,subprocess,time,os
root=pathlib.Path(__file__).resolve().parents[2];run=root/'research/sniper87/runtime-online-connection';run.mkdir(exist_ok=True)
exe=root/'code/platform/build/online-connection-test.exe'
subprocess.run(['i686-w64-mingw32-gcc','-std=c11','-O2','-g','-Wall','-Wextra','-Werror',str(root/'code/tests/online_connection_runtime_test.c'),*[str(root/'code/platform'/n) for n in ['online_connection.c','network_socket.c']],*[str(root/'code/src'/f'network_{n}.c') for n in ['client','login','game_packets','game_receive','send','receive','transport','compression','lifecycle','checksum']],str(root/'code/src/level.c'),str(root/'code/platform/build/resources.o'),'-o',str(exe),'-lws2_32','-lcomctl32','-luser32'],check=True)
tcp=socket.socket();tcp.bind(('127.0.0.1',0));tcp.listen();tcp.settimeout(90)
udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);udp.bind(('127.0.0.1',0));udp.settimeout(30)
errors=[];seen=[];level=b'\x01\x20\0\x01\x02\x30\0\x02';arena=b'Fixture,127.0.0.1,382\n'
def checksum(b):return sum(struct.unpack('<'+'I'*(len(b)//4),b[:len(b)//4*4]))&0xffffffff
# Seed old files to verify preservation and forced refresh.
cache=run/'online-cache';cache.mkdir(exist_ok=True)
name=('127001'+str(tcp.getsockname()[1])+'.lvl')[-12:];(cache/name).write_bytes(b'old-map');(cache/'arena.lst').write_bytes(b'old-arena');(cache/'news.txt').write_bytes(b'old-news')
def peer():
    try:
        for cycle in range(3):
            conn,_=tcp.accept();conn.settimeout(30);hello=b''
            while len(hello)<6:hello+=conn.recv(6-len(hello))
            addr=(socket.inet_ntoa(hello[:4]),struct.unpack('!H',hello[4:])[0]);conn.sendall(socket.inet_aton('127.0.0.1')+struct.pack('!H',udp.getsockname()[1]));assert conn.recv(1)==b'';conn.close()
            sequence=1000;dedup=set();bye=0
            def reliable(body):
                nonlocal sequence
                udp.sendto(b'\0\2'+struct.pack('<IH',sequence,(sequence-1000)%256)+body,addr);sequence+=1
            while bye<4:
                data,source=udp.recvfrom(4096)
                if source!=addr:continue
                if data[:2]==b'\0\0':udp.sendto(b'\0\1'+data[2:6]+struct.pack('<I',int(time.monotonic()*100)%0x19999999),addr);continue
                if data[:2]==b'\0\3':continue
                if data==b'\0\4':bye+=1;continue
                if data[:2]==b'\0\2':
                    seq=struct.unpack('<I',data[2:6])[0];udp.sendto(b'\0\3'+data[2:8],addr)
                    if seq in dedup:continue
                    dedup.add(seq);data=data[8:]
                op=data[0];seen.append((cycle,op))
                if op==9:
                    assert len(data)==98 and data[2:10]==b'Pilot87\0';status=1 if cycle==0 and data[1]==0 else 0
                    reliable(b'\x0a'+bytes([status])+struct.pack('<iIII',87,sum(level)+(cycle==2),0x68fc54a0,checksum(arena)))
                elif op in (12,18,13):
                    body={12:level,18:arena,13:b'fixture news'}[op]
                    if op==13:reliable(b'\x07\x07\xff\xffduring download\0')
                    reliable(b'\x0d'+struct.pack('<i',len(body)));reliable(b'\x0c'+struct.pack('<i',0)+body);reliable(b'\x0c'+struct.pack('<i',-1))
                else:raise AssertionError((op,data))
    except BaseException as e:errors.append(e)
t=threading.Thread(target=peer,daemon=True);t.start();env=os.environ.copy();env.update(WINEPREFIX=str(run/'wine'),WINEDEBUG='-all',LIBGL_ALWAYS_SOFTWARE='1')
try:result=subprocess.run(['xvfb-run','-a','wine',str(exe),str(tcp.getsockname()[1])],cwd=run,env=env,capture_output=True,text=True,timeout=100)
finally:subprocess.run(['wineserver','-k'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
(run/'result.txt').write_text(result.stdout);(run/'wine.log').write_text(result.stderr);t.join(timeout=2);tcp.close();udp.close()
assert result.returncode==0,(result.returncode,result.stdout,result.stderr,errors)
assert not errors and not t.is_alive(),errors
assert seen==[(0,9),(0,9),(0,12),(0,18),(0,13),(1,9),(2,9),(2,12)],seen
assert (cache/name).read_bytes()==level and (cache/'arena.lst').read_bytes()==arena
backups=[p.read_bytes() for p in cache.glob('old*.tmp')];assert all(b in backups for b in [b'old-map',b'old-arena',b'old-news'])
print(result.stdout.strip());print('PASS loopback peer order, checksums, cache reuse, and preserved prior cache files')

for filename,op,body,expected in [(name,12,level,sum(level)),('arena.lst',18,arena,checksum(arena)),('news.txt',13,b'fixture news',None)]:
    metadata=json.loads((cache/(filename+'.source.json')).read_text())
    assert metadata['hostname']=='127.0.0.1' and metadata['tcp_port'].isdigit()
    assert metadata['request_opcode']==op and metadata['bytes']==len(body)
    actual=sum(body[:len(body)//4*4]) if op==12 else checksum(body)
    assert metadata['actual_checksum']==f'{actual:08x}'
    assert metadata['expected_checksum']==(f'{expected:08x}' if expected is not None else None)
print('PASS event_user forwarding and download source/checksum sidecars')
